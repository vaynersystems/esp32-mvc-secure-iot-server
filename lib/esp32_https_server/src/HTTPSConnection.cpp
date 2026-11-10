#include "HTTPSConnection.hpp"
#include "TLSClientHello.hpp"
#include <errno.h>
#include <stddef.h>
#include "mbedtls/ssl.h"
#include "mbedtls/ssl_cache.h"
#include "mbedtls/net_sockets.h"

// How long a new TLS connection may stay silent before it is dropped.
#ifndef HTTPS_HANDSHAKE_IDLE_MS
#define HTTPS_HANDSHAKE_IDLE_MS 500
#endif
// How long to wait for the rest of a ClientHello split over several TCP segments.
#ifndef HTTPS_SOCKET_IO_TIMEOUT_MS
#define HTTPS_SOCKET_IO_TIMEOUT_MS 3000
#endif
#ifndef HTTPS_HELLO_MIN_SEGMENT
#define HTTPS_HELLO_MIN_SEGMENT 536
#endif
#ifndef HTTPS_HELLO_WAIT_MS
#define HTTPS_HELLO_WAIT_MS 1500
#endif
// Per-read timeout while the handshake runs. SSL_accept blocks the whole server and has no
// overall deadline, so this bounds how long a client trickling its handshake can stall it.
#ifndef HTTPS_HANDSHAKE_READ_TIMEOUT_MS
#define HTTPS_HANDSHAKE_READ_TIMEOUT_MS 1000
#endif

namespace httpsserver {

HTTPSConnection::ServerNameFilter HTTPSConnection::_serverNameFilter = NULL;

// ---- TLS session resumption -------------------------------------------------------------
// ESP-IDF's OpenSSL wrapper builds a fresh mbedtls config per connection and does not expose
// session caching, so every connection paid the full key exchange and signature (~0.5 s on
// the ESP32-S3, which blocks the whole server). A returning browser can resume a cached
// session instead, which costs only a few milliseconds.
//
// The wrapper's per-connection state (struct ssl_pm, private to ssl_pm.c) begins with two
// mbedtls_net_context fields followed by the mbedtls_ssl_config; this offset was read from
// the compiled library (ssl_pm_new passes ssl_pm+8 to mbedtls_ssl_config_init). The static
// asserts stop the build if a different SDK changes the layout.
struct EspSslPmPrefix {
  mbedtls_net_context fd;
  mbedtls_net_context clientFd;
  mbedtls_ssl_config conf;
};
static_assert(offsetof(EspSslPmPrefix, conf) == 8, "ESP-IDF ssl_pm layout changed: session cache hook must be revisited");
static_assert(sizeof(mbedtls_ssl_config) == 232, "mbedtls_ssl_config size changed: session cache hook must be revisited");

#ifndef HTTPS_SESSION_CACHE_ENTRIES
#define HTTPS_SESSION_CACHE_ENTRIES 16
#endif
#ifndef HTTPS_SESSION_CACHE_SECONDS
#define HTTPS_SESSION_CACHE_SECONDS 3600
#endif

static void enableSessionCache(SSL* ssl) {
  static mbedtls_ssl_cache_context cache;
  static bool cacheReady = false;
  if (!cacheReady) {
    mbedtls_ssl_cache_init(&cache);
    mbedtls_ssl_cache_set_max_entries(&cache, HTTPS_SESSION_CACHE_ENTRIES);
    mbedtls_ssl_cache_set_timeout(&cache, HTTPS_SESSION_CACHE_SECONDS);
    cacheReady = true;
  }
  if (ssl == NULL || ssl->ssl_pm == NULL) return;
  mbedtls_ssl_config* conf = &((EspSslPmPrefix*)ssl->ssl_pm)->conf;
  mbedtls_ssl_conf_session_cache(conf, &cache, mbedtls_ssl_cache_get, mbedtls_ssl_cache_set);
}

void HTTPSConnection::setServerNameFilter(ServerNameFilter filter) {
  _serverNameFilter = filter;
}

/**
 * Peeks at the ClientHello (nothing is consumed) and asks the filter about the server name.
 * Allows the connection when there is no filter, no server name, or the hello is unreadable,
 * so only a name the filter rejects is refused.
 */
bool HTTPSConnection::serverNameAllowed(int socket) {
  // Wait for the complete ClientHello record before doing any TLS work. A client that sends it
  // slowly (or not at all) is dropped here instead of holding the single server task inside
  // the blocking SSL_accept. Also lets the server-name filter refuse foreign names cheaply.
  static uint8_t hello[4096]; // connections are accepted one at a time by the server task
  char serverName[256];
  unsigned long start = millis();
  TLSClientHelloResult result = TLS_HELLO_INCOMPLETE;
  while (true) {
    int received = recv(socket, hello, sizeof(hello), MSG_PEEK | MSG_DONTWAIT);
    if (received > 0) result = tlsExtractSNI(hello, received, serverName, sizeof(serverName));
    if (result != TLS_HELLO_INCOMPLETE) break;
    if (received == 0) return false;                     // client closed
    if (received == (int)sizeof(hello)) break;            // hello larger than the peek buffer: let TLS decide
    // lwIP's MSG_PEEK only returns the first buffered TCP segment, so a ClientHello larger than
    // one segment (modern ones are ~1.5-1.8 KB) can never be peeked whole. A full-size first
    // segment shows a real client; a slow-trickling one never gets this far within the deadline.
    if (received >= HTTPS_HELLO_MIN_SEGMENT) break;
    if (millis() - start > HTTPS_HELLO_WAIT_MS) {
      HTTPS_LOGI("Incomplete TLS ClientHello after %d ms, closing. FID=%d", HTTPS_HELLO_WAIT_MS, socket);
      return false;
    }
    delay(5);
  }
  if (result == TLS_HELLO_NOT_TLS) return false;
  if (result != TLS_HELLO_SNI || _serverNameFilter == NULL) return true;
  if (_serverNameFilter(serverName)) return true;
  HTTPS_LOGI("Refusing TLS connection for %s before handshake. FID=%d", serverName, socket);
  return false;
}


HTTPSConnection::HTTPSConnection(ResourceResolver * resResolver):
  HTTPConnection(resResolver) {
  _ssl = NULL;
}

HTTPSConnection::~HTTPSConnection() {
  // Close the socket
  closeConnection();
}

bool HTTPSConnection::isSecure() {
  return true;
}

/**
 * Initializes the connection from a server socket.
 *
 * The call WILL BLOCK if accept(serverSocketID) blocks. So use select() to check for that in advance.
 */
int HTTPSConnection::initialize(int serverSocketID, SSL_CTX * sslCtx, HTTPHeaders *defaultHeaders) {
  if (_connectionState == STATE_UNDEFINED) {
    // Let the base class connect the plain tcp socket
    int resSocket = HTTPConnection::initialize(serverSocketID, defaultHeaders);

    // Build up SSL Connection context if the socket has been created successfully
    if (resSocket >= 0) {

      // SSL_accept blocks the whole server (HTTP included) until the handshake ends.
      // Browsers pre-open connections and phones probe HTTPS without always sending a
      // ClientHello, so only start the handshake once the client has sent something.
      // A connection that stays idle is closed; a real client simply connects again.
      fd_set readfds;
      FD_ZERO(&readfds);
      FD_SET(resSocket, &readfds);
      struct timeval idleWait;
      idleWait.tv_sec = HTTPS_HANDSHAKE_IDLE_MS / 1000;
      idleWait.tv_usec = (HTTPS_HANDSHAKE_IDLE_MS % 1000) * 1000;
      if (select(resSocket + 1, &readfds, NULL, NULL, &idleWait) <= 0) {
        HTTPS_LOGI("No TLS ClientHello within %d ms, closing. FID=%d", HTTPS_HANDSHAKE_IDLE_MS, resSocket);
        _connectionState = STATE_ERROR;
        _clientState = CSTATE_ACTIVE;
        closeConnection();
        return -1;
      }

      if (!serverNameAllowed(resSocket)) {
        _connectionState = STATE_ERROR;
        _clientState = CSTATE_ACTIVE;
        closeConnection();
        return -1;
      }

      _ssl = SSL_new(sslCtx);

      if (_ssl) {
        enableSessionCache(_ssl);
        // Bind SSL to the socket
        int success = SSL_set_fd(_ssl, resSocket);
        if (success) {

          // Perform the handshake
          unsigned long handshakeStart = millis();
          struct timeval handshakeTimeout = { HTTPS_HANDSHAKE_READ_TIMEOUT_MS / 1000, (HTTPS_HANDSHAKE_READ_TIMEOUT_MS % 1000) * 1000 };
          setsockopt(resSocket, SOL_SOCKET, SO_RCVTIMEO, &handshakeTimeout, sizeof(handshakeTimeout));
          errno = 0;
          success = SSL_accept(_ssl);
          // Back to the normal per-read timeout for the rest of the connection.
          struct timeval ioTimeout = { HTTPS_SOCKET_IO_TIMEOUT_MS / 1000, (HTTPS_SOCKET_IO_TIMEOUT_MS % 1000) * 1000 };
          setsockopt(resSocket, SOL_SOCKET, SO_RCVTIMEO, &ioTimeout, sizeof(ioTimeout));
          if (success == 1) {
            // A resumed session takes a few ms; a full handshake ~0.5 s.
            HTTPS_LOGI("TLS handshake done in %lu ms. FID=%d", millis() - handshakeStart, resSocket);
            return resSocket;
          } else {
            // errno tells a timeout (EAGAIN: client went quiet) from a reset by the client or an
            // alert (errno 0, e.g. the client rejected the certificate).
            int sslError = SSL_get_error(_ssl, success);
            int socketError = errno;
            HTTPS_LOGE("SSL_accept failed after %lu ms (ret=%d ssl_error=%d errno=%d). Aborting handshake. FID=%d",
              millis() - handshakeStart, success, sslError, socketError, resSocket);
          }
        } else {
          HTTPS_LOGE("SSL_set_fd failed. Aborting handshake. FID=%d", resSocket);
        }
      } else {
        HTTPS_LOGE("SSL_new failed. Aborting handshake. FID=%d", resSocket);
      }

    } else {
      HTTPS_LOGE("Could not accept() new connection. FID=%d", resSocket);
    }

    _connectionState = STATE_ERROR;
    _clientState = CSTATE_ACTIVE;

    // This will only be called if the connection could not be established and cleanup
    // variables like _ssl etc.
    closeConnection();
  }
  // Error: The connection has already been established or could not be established
  return -1;
}


void HTTPSConnection::closeConnection() {

  // FIXME: Copy from HTTPConnection, could be done better probably
  if (_connectionState != STATE_ERROR && _connectionState != STATE_CLOSED) {

    // First call to closeConnection - set the timestamp to calculate the timeout later on
    if (_connectionState != STATE_CLOSING) {
      _shutdownTS = millis();
    }

    // Set the connection state to closing. We stay in closing as long as SSL has not been shutdown
    // correctly
    _connectionState = STATE_CLOSING;
  }

  // Try to tear down SSL while we are in the _shutdownTS timeout period or if an error occurred
  if (_ssl) {
    if(_connectionState == STATE_ERROR || SSL_shutdown(_ssl) == 0) {
      // SSL_shutdown will return 1 as soon as the client answered with close notify
      // This means we are safe to close the socket
      SSL_free(_ssl);
      _ssl = NULL;
    } else if (_shutdownTS + HTTPS_SHUTDOWN_TIMEOUT < millis()) {
      // The timeout has been hit, we force SSL shutdown now by freeing the context
      SSL_free(_ssl);
      _ssl = NULL;
      HTTPS_LOGW("SSL_shutdown did not receive close notification from the client");
      _connectionState = STATE_ERROR;
    }
  }

  // If SSL has been brought down, close the socket
  if (!_ssl) {
    HTTPConnection::closeConnection();
  }
}



size_t HTTPSConnection::writeBuffer(byte* buffer, size_t length) {
  size_t written  = 0;

  if (_ssl)
  {
    int sslWriteRet = SSL_write(_ssl, buffer, length);
  
    if (0 < sslWriteRet)
    {
      written = sslWriteRet;
    }
  }

  return written;
}

size_t HTTPSConnection::readBytesToBuffer(byte* buffer, size_t length) {
  size_t read = 0;

  if (_ssl)
  {
    int sslReadRet = SSL_read(_ssl, buffer, length);

    if (0 < sslReadRet)
    {
      read = sslReadRet;
    }
  }

  return read;
}

size_t HTTPSConnection::pendingByteCount() {
  size_t pending = 0;

  if (_ssl)
  {
    int sslPending = SSL_pending(_ssl);

    if (0 < sslPending)
    {
      pending = sslPending;
    }
  }

  return pending;
}



bool HTTPSConnection::canReadData() {
  return HTTPConnection::canReadData() || (SSL_pending(_ssl) > 0);
}

} /* namespace httpsserver */
