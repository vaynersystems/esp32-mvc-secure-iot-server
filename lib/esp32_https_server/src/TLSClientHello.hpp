#ifndef SRC_TLSCLIENTHELLO_HPP_
#define SRC_TLSCLIENTHELLO_HPP_
// Reads the server name (SNI) from a TLS ClientHello without doing any cryptography,
// so a server can drop connections meant for other sites before an expensive handshake.
// Pure function, no sockets: testable off-device.
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace httpsserver {

enum TLSClientHelloResult {
  TLS_HELLO_INCOMPLETE = -1, // need more bytes (the hello can span several TCP segments)
  TLS_HELLO_NOT_TLS    = -2, // not a TLS handshake record / malformed
  TLS_HELLO_NO_SNI     = 0,  // complete hello without a server name (e.g. client used an IP address)
  TLS_HELLO_SNI        = 1,  // server name copied to the output buffer
};

static inline uint16_t tlsRead16(const uint8_t* p) { return (uint16_t)((p[0] << 8) | p[1]); }

/// @param data bytes received so far (from MSG_PEEK)
/// @param name receives the host name, NUL terminated (truncated to nameCapacity-1)
static inline TLSClientHelloResult tlsExtractSNI(const uint8_t* data, size_t length, char* name, size_t nameCapacity) {
  if (nameCapacity) name[0] = 0;
  if (length < 5) return TLS_HELLO_INCOMPLETE;
  if (data[0] != 0x16 || data[1] != 0x03) return TLS_HELLO_NOT_TLS; // handshake record, TLS 1.x
  size_t recordLength = tlsRead16(data + 3);
  size_t recordEnd = 5 + recordLength;
  if (length < 5 + 4) return TLS_HELLO_INCOMPLETE;
  if (data[5] != 0x01) return TLS_HELLO_NOT_TLS; // ClientHello
  size_t helloLength = ((size_t)data[6] << 16) | ((size_t)data[7] << 8) | data[8];
  size_t helloEnd = 9 + helloLength;
  // A ClientHello larger than one record is legal but rare; parse what the first record holds.
  size_t end = helloEnd < recordEnd ? helloEnd : recordEnd;
  if (length < end) return TLS_HELLO_INCOMPLETE;

  size_t pos = 9 + 2 + 32;                                   // version, random
  if (pos + 1 > end) return TLS_HELLO_NOT_TLS;
  pos += 1 + data[pos];                                      // session id
  if (pos + 2 > end) return TLS_HELLO_NOT_TLS;
  pos += 2 + tlsRead16(data + pos);                          // cipher suites
  if (pos + 1 > end) return TLS_HELLO_NOT_TLS;
  pos += 1 + data[pos];                                      // compression methods
  if (pos == end) return TLS_HELLO_NO_SNI;                   // no extensions at all
  if (pos + 2 > end) return TLS_HELLO_NOT_TLS;
  size_t extensionsEnd = pos + 2 + tlsRead16(data + pos);
  pos += 2;
  if (extensionsEnd > end) return TLS_HELLO_NOT_TLS;

  while (pos + 4 <= extensionsEnd) {
    uint16_t type = tlsRead16(data + pos);
    size_t extLength = tlsRead16(data + pos + 2);
    pos += 4;
    if (pos + extLength > extensionsEnd) return TLS_HELLO_NOT_TLS;
    if (type == 0x0000) {                                    // server_name
      size_t p = pos;
      if (p + 2 > pos + extLength) return TLS_HELLO_NOT_TLS;
      size_t listEnd = p + 2 + tlsRead16(data + p);
      p += 2;
      if (listEnd > pos + extLength) return TLS_HELLO_NOT_TLS;
      while (p + 3 <= listEnd) {
        uint8_t nameType = data[p];
        size_t nameLength = tlsRead16(data + p + 1);
        p += 3;
        if (p + nameLength > listEnd) return TLS_HELLO_NOT_TLS;
        if (nameType == 0) {                                 // host_name
          size_t n = nameLength < nameCapacity - 1 ? nameLength : nameCapacity - 1;
          memcpy(name, data + p, n);
          name[n] = 0;
          return TLS_HELLO_SNI;
        }
        p += nameLength;
      }
      return TLS_HELLO_NO_SNI;
    }
    pos += extLength;
  }
  return TLS_HELLO_NO_SNI;
}

} // namespace httpsserver
#endif
