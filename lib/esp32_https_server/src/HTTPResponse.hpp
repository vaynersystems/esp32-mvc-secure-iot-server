#ifndef SRC_HTTPRESPONSE_HPP_
#define SRC_HTTPRESPONSE_HPP_

#include <Arduino.h>
#include <string>
// Arduino declares it's own min max, incompatible with the stl...
#undef min
#undef max
#undef write
#include <vector>

#include <openssl/ssl.h>
//#include "esp_tls.h"

#include "util.hpp"

#include "ConnectionContext.hpp"
#include "HTTPHeaders.hpp"
#include "HTTPHeader.hpp"

namespace httpsserver {

/**
 * \brief Represents the response stream of an HTTP request
 */
class HTTPResponse : public Print {
public:
  HTTPResponse(ConnectionContext * con);
  virtual ~HTTPResponse();

  void setStatusCode(uint16_t statusCode);
  void setStatusText(std::string const &statusText);
  uint16_t getStatusCode();
  std::string getStatusText();
  void setHeader(std::string const &name, std::string const &value);
  std::string getHeader(std::string const &name);
  bool isHeaderWritten();

  void printStd(std::string const &str);

  // From Print:
  size_t write(const uint8_t *buffer, size_t size);
  size_t write(uint8_t);

  void error();

  bool isResponseBuffered();
  void finalize();
  // True when the response was streamed (too large for the buffer) with a Content-Length
  // set by the handler and exactly that many body bytes were sent: the connection can be
  // kept alive for the next request.
  bool isCompleteWithKnownLength();

  ConnectionContext * _con;
  
private:
  void printHeader();
  void printInternal(const std::string &str, bool skipBuffer = false);
  size_t writeBytesInternal(const void * data, int length, bool skipBuffer = false);
  void drainBuffer(bool onOverflow = false);

  uint16_t _statusCode;
  std::string _statusText;
  HTTPHeaders _headers;
  bool _headerWritten;
  bool _isError;
  size_t _bodyBytesSent;   // body bytes written to the connection (headers excluded)

  // Response cache
  byte * _responseCache;
  size_t _responseCacheSize;
  size_t _responseCachePointer;
};

} /* namespace httpsserver */

#endif /* SRC_HTTPRESPONSE_HPP_ */
