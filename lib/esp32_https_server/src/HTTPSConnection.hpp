#ifndef SRC_HTTPSCONNECTION_HPP_
#define SRC_HTTPSCONNECTION_HPP_

#include <Arduino.h>

#include <string>

// Required for SSL
#include "openssl/ssl.h"
#undef read

// Required for sockets
#include "lwip/netdb.h"
#undef read
#include "lwip/sockets.h"

#include "HTTPSServerConstants.hpp"
#include "HTTPConnection.hpp"
#include "HTTPHeaders.hpp"
#include "HTTPHeader.hpp"
#include "ResourceResolver.hpp"
#include "ResolvedResource.hpp"
#include "ResourceNode.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

namespace httpsserver {

/**
 * \brief Connection class for an open TLS-enabled connection to an HTTPSServer
 */
class HTTPSConnection : public HTTPConnection {
public:
  HTTPSConnection(ResourceResolver * resResolver);
  virtual ~HTTPSConnection();

  virtual int initialize(int serverSocketID, SSL_CTX * sslCtx, HTTPHeaders *defaultHeaders);
  virtual void closeConnection();
  virtual bool isSecure();

  // Optional check of the server name (SNI) a client asks for, made before the TLS
  // handshake. Returning false closes the connection without any cryptography.
  typedef bool (*ServerNameFilter)(const char* serverName);
  static void setServerNameFilter(ServerNameFilter filter);

protected:
  friend class HTTPRequest;
  friend class HTTPResponse;

  virtual size_t readBytesToBuffer(byte* buffer, size_t length);
  virtual size_t pendingByteCount();
  virtual bool canReadData();
  virtual size_t writeBuffer(byte* buffer, size_t length);

private:
  // SSL context for this connection
  SSL * _ssl;
  static ServerNameFilter _serverNameFilter;
  bool serverNameAllowed(int socket);

};

} /* namespace httpsserver */

#endif /* SRC_HTTPSCONNECTION_HPP_ */
