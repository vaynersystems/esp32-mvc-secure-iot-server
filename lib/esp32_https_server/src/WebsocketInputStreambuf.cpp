#include "WebsocketInputStreambuf.hpp"

namespace httpsserver {
/**
 * @brief Create a Web Socket input record streambuf
 * @param [in] socket The socket we will be reading from.
 * @param [in] dataLength The size of a record.
 * @param [in] bufferSize The size of the buffer we wish to allocate to hold data.
 */
WebsocketInputStreambuf::WebsocketInputStreambuf(
  ConnectionContext   *con,
  size_t dataLength,
  uint8_t *pMask,
  size_t bufferSize
) {
  _con     = con;    // The socket we will be reading from
  _dataLength = dataLength; // The size of the record we wish to read.
  _pMask      = pMask;
  _bufferSize = bufferSize; // The size of the buffer used to hold data
  _sizeRead   = 0;          // The size of data read from the socket
  _buffer = new char[bufferSize]; // Create the buffer used to hold the data read from the socket.

  setg(_buffer, _buffer, _buffer); // Set the initial get buffer pointers to no data.
}

WebsocketInputStreambuf::~WebsocketInputStreambuf() {
  //FIXME: Call order incorrect? discard() uses _buffer
  delete[] _buffer;
  discard();
}


/**
 * @brief Discard data for the record that has not yet been read.
 *
 * We are working on a logical fixed length record in a socket stream.  This means that we know in advance
 * how big the record should be.  If we have read some data from the stream and no longer wish to consume
 * any further, we have to discard the remaining bytes in the stream before we can get to process the
 * next record.  This function discards the remainder of the data.
 *
 * For example, if our record size is 1000 bytes and we have read 700 bytes and determine that we no
 * longer need to continue, we can't just stop.  There are still 300 bytes in the socket stream that
 * need to be consumed/discarded before we can move on to the next record.
 */
#ifndef HTTPS_WS_READ_TIMEOUT_MS
#define HTTPS_WS_READ_TIMEOUT_MS 2000
#endif

bool WebsocketInputStreambuf::discard() {
  // Counts only bytes actually read (it used to count one per call even when nothing arrived,
  // which desynchronised the stream or spun for hours on a huge declared length).
  uint8_t scratch[64];
  unsigned long start = millis();
  while(_sizeRead < _dataLength) {
    size_t want = _dataLength - _sizeRead;
    if (want > sizeof(scratch)) want = sizeof(scratch);
    size_t got = _con->readBuffer(scratch, want);
    if (got == 0) {
      if (millis() - start > HTTPS_WS_READ_TIMEOUT_MS) return false;
      delay(2);
      continue;
    }
    _sizeRead += got;
  }
  return true;
} // WebsocketInputStreambuf::discard


/**
 * @brief Get the size of the expected record.
 * @return The size of the expected record.
 */
size_t WebsocketInputStreambuf::getRecordSize() {
  return _dataLength;
} // WebsocketInputStreambuf::getRecordSize

/**
 * @brief Handle the request to read data from the stream but we need more data from the source.
 *
 */
WebsocketInputStreambuf::int_type WebsocketInputStreambuf::underflow() {
  HTTPS_LOGD(">> WebSocketInputStreambuf.underflow()");

  // If we have already read as many bytes as our record definition says we should read
  // then don't attempt to ready any further.
  if (_sizeRead >= getRecordSize()) {
    HTTPS_LOGD("<< WebSocketInputStreambuf.underflow(): Already read maximum");
    return EOF;
  }

  // We wish to refill the buffer.  We want to read data from the socket.  We want to read either
  // the size of the buffer to fill it or the maximum number of bytes remaining to be read.
  // We will choose which ever is smaller as the number of bytes to read into the buffer.
  int remainingBytes = getRecordSize()-_sizeRead;
  size_t sizeToRead;
  if (remainingBytes < _bufferSize) {
    sizeToRead = remainingBytes;
  } else {
    sizeToRead = _bufferSize;
  }

  HTTPS_LOGD("WebSocketInputRecordStreambuf - getting next buffer of data; size request: %d", sizeToRead);
  // The payload can arrive in several TCP segments: wait briefly for the next one instead of
  // ending the message early (which cut messages short and desynchronised the stream).
  int bytesRead = _con->readBuffer((uint8_t*)_buffer, sizeToRead);
  unsigned long waitStart = millis();
  while (bytesRead == 0 && millis() - waitStart < HTTPS_WS_READ_TIMEOUT_MS) {
    delay(2);
    bytesRead = _con->readBuffer((uint8_t*)_buffer, sizeToRead);
  }
  if (bytesRead == 0) {
    HTTPS_LOGD("<< WebSocketInputRecordStreambuf.underflow(): Read 0 bytes");
    return EOF;
  }

  // If the WebSocket frame shows that we have a mask bit set then we have to unmask the data.
  if (_pMask != nullptr) {
    for (int i=0; i<bytesRead; i++) {
      _buffer[i] = _buffer[i] ^ _pMask[(_sizeRead+i)%4];
    }
  }

  _sizeRead += bytesRead;  // Increase the count of number of bytes actually read from the source.

  setg(_buffer, _buffer, _buffer + bytesRead); // Change the buffer pointers to reflect the new data read.
  HTTPS_LOGD("<< WebSocketInputRecordStreambuf.underflow(): got %d bytes", bytesRead);
  return traits_type::to_int_type(*gptr());
} // underflow

}
