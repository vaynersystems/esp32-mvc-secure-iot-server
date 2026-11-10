#include "WebsocketHandler.hpp"
#ifndef TAG
#define TAG "ARDUINO"
#endif
namespace httpsserver {

/**
 * @brief Dump the content of the WebSocket frame for debugging.
 * @param [in] frame The frame to dump.
 */
static void dumpFrame(WebsocketFrame frame) {
  std::string opcode = std::string("Unknown");
  switch(frame.opCode) {
    case WebsocketHandler::OPCODE_BINARY: opcode = std::string("BINARY"); break;
    case WebsocketHandler::OPCODE_CONTINUE: opcode = std::string("CONTINUE"); break;
    case WebsocketHandler::OPCODE_CLOSE: opcode = std::string("CLOSE"); break;
    case WebsocketHandler::OPCODE_PING: opcode = std::string("PING"); break;
    case WebsocketHandler::OPCODE_PONG: opcode = std::string("PONG"); break;
    case WebsocketHandler::OPCODE_TEXT: opcode = std::string("TEXT"); break;
  }
  ESP_LOGI(
    "",
    "Fin: %d, OpCode: %d (%s), Mask: %d, Len: %d",
    (int)frame.fin,
    (int)frame.opCode,
    opcode.c_str(),
    (int)frame.mask,
    (int)frame.len
  );
}

WebsocketHandler::WebsocketHandler() {
  _con = nullptr;
  _receivedClose = false;
  _sentClose = false;
}

WebsocketHandler::~WebsocketHandler() {

} // ~WebSocketHandler()


/**
* @brief The default onClose handler.
* If no over-riding handler is provided for the "close" event, this method is called.
*/
void WebsocketHandler::onClose() {
  HTTPS_LOGD("WebsocketHandler close()");
}

/**
* @brief The default onData handler.
* If no over-riding handler is provided for the "message" event, this method is called.
* A particularly useful pattern for using onMessage is:
* ```
* std::stringstream buffer;
* buffer << pWebSocketInputRecordStreambuf;
* ```
* This will read the whole message into the string stream.
*/
void WebsocketHandler::onMessage(WebsocketInputStreambuf* pWebsocketInputStreambuf) { //, Websocket *pWebSocket) {
  HTTPS_LOGD("WebsocketHandler onMessage()");
}


/**
* @brief The default onError handler.
* If no over-riding handler is provided for the "error" event, this method is called.
*/
void WebsocketHandler::onError(std::string error) {
  HTTPS_LOGD("WebsocketHandler onError()");
}

void WebsocketHandler::initialize(ConnectionContext * con) {
  _con = con;
}

void WebsocketHandler::loop() {
  if(read() < 0) {
    close();
  }
}

#ifndef HTTPS_WS_MAX_PAYLOAD
#define HTTPS_WS_MAX_PAYLOAD 4096
#endif

int WebsocketHandler::read() {
  // 1. Gather the complete frame header (it may arrive in pieces).
  if (_headerLength < 2) {
    int got = _con->readBuffer(_header + _headerLength, 2 - _headerLength);
    if (got <= 0) return 0;
    _headerLength += got;
    if (_headerLength < 2) return 0;
  }
  uint8_t opCode = _header[0] & 0x0F;
  bool masked = (_header[1] & 0x80) != 0;
  uint8_t len7 = _header[1] & 0x7F;
  uint8_t needed = 2 + (len7 == 126 ? 2 : len7 == 127 ? 8 : 0) + (masked ? 4 : 0);
  if (_headerLength < needed) {
    int got = _con->readBuffer(_header + _headerLength, needed - _headerLength);
    if (got > 0) _headerLength += got;
    if (_headerLength < needed) return 0; // rest arrives later
  }
  _headerLength = 0; // header complete; the next call starts a new frame

  // 2. Payload length, big-endian; reject anything larger than the application handles.
  uint64_t payloadLen64 = len7;
  uint8_t pos = 2;
  if (len7 == 126) {
    payloadLen64 = ((uint64_t)_header[2] << 8) | _header[3];
    pos = 4;
  } else if (len7 == 127) {
    payloadLen64 = 0;
    for (int i = 0; i < 8; i++) payloadLen64 = (payloadLen64 << 8) | _header[2 + i];
    pos = 10;
  }
  uint8_t mask[4] = {0, 0, 0, 0};
  if (masked) memcpy(mask, _header + pos, 4);
  if (payloadLen64 > HTTPS_WS_MAX_PAYLOAD) {
    HTTPS_LOGW("WS payload of %u bytes exceeds the %d byte limit, closing", (unsigned)payloadLen64, HTTPS_WS_MAX_PAYLOAD);
    close(CLOSE_TOO_BIG, "");
    return -1;
  }
  size_t payloadLen = (size_t)payloadLen64;
  if (opCode >= 0x08 && payloadLen > 125) { // control frames are limited to 125 bytes
    close(CLOSE_PROTOCOL_ERROR, "");
    return -1;
  }

  // 3. Handle the frame. Every branch consumes the payload so the stream stays aligned.
  WebsocketInputStreambuf streambuf(_con, payloadLen, masked ? mask : nullptr);
  switch(opCode) {
    case OPCODE_TEXT:
    case OPCODE_BINARY:
      onMessage(&streambuf);
      break;
    case OPCODE_CLOSE:
      streambuf.discard();
      _receivedClose = true;
      onClose();
      return -1;
    case OPCODE_CONTINUE: // fragmented messages are not supported: skip the fragment
    case OPCODE_PING:     // (no pong is sent; browsers do not require one)
    case OPCODE_PONG:
      break;
    default:
      HTTPS_LOGW("WebSocketReader: Unknown opcode: %d", opCode);
      close(CLOSE_PROTOCOL_ERROR, "");
      return -1;
  }
  if (!streambuf.discard()) {
    // The payload did not arrive in time: the frame stream can no longer be trusted.
    close(CLOSE_PROTOCOL_ERROR, "");
    return -1;
  }
  return 0;
}  // Websocket::read

/**
 * @brief Close the Web socket
 * @param [in] status The code passed in the close request.
 * @param [in] message A clarification message on the close request.
 */
void WebsocketHandler::close(uint16_t status, std::string message) {
  HTTPS_LOGD("Websocket close()");

  _sentClose = true;              // Flag that we have sent a close request.

  WebsocketFrame frame;           // Build the web socket frame indicating a close request.
  frame.fin    = 1;
  frame.rsv1   = 0;
  frame.rsv2   = 0;
  frame.rsv3   = 0;
  frame.opCode = OPCODE_CLOSE;
  frame.mask   = 0;
  frame.len    = message.length() + 2;
  int rc = _con->writeBuffer((uint8_t *)&frame, sizeof(frame));

  if (rc > 0) {
    rc = _con->writeBuffer((byte *) &status, 2);
  }

  if (rc > 0) {
    _con->writeBuffer((byte *) message.data(), message.length());
  }
} // Websocket::close

/**
 * @brief Send data down the web socket
 * See the WebSocket spec (RFC6455) section "6.1 Sending Data".
 * We build a WebSocket frame, send the frame followed by the data.
 * @param [in] data The data to send down the WebSocket.
 * @param [in] sendType The type of payload.  Either SEND_TYPE_TEXT or SEND_TYPE_BINARY.
 */
void WebsocketHandler::send(std::string data, uint8_t sendType) {
  HTTPS_LOGD(">> Websocket.send(): length=%d", data.length());
  WebsocketFrame frame;
  frame.fin    = 1;
  frame.rsv1   = 0;
  frame.rsv2   = 0;
  frame.rsv3   = 0;
  frame.opCode = sendType==SEND_TYPE_TEXT?OPCODE_TEXT:OPCODE_BINARY;
  frame.mask   = 0;
  if (data.length() < 126) {
    frame.len = data.length();
    _con->writeBuffer((uint8_t *)&frame, sizeof(frame));
  } else {
    frame.len = 126;
    _con->writeBuffer((uint8_t *)&frame, sizeof(frame));
    uint16_t net_len = htons((uint16_t)data.length());
    _con->writeBuffer((uint8_t *)&net_len, sizeof(uint16_t));  // Convert to network byte order from host byte order
  }
  _con->writeBuffer((uint8_t*)data.data(), data.length());
  HTTPS_LOGD("<< Websocket.send()");
} // Websocket::send


/**
 * @brief Send data down the web socket
 * See the WebSocket spec (RFC6455) section "6.1 Sending Data".
 * We build a WebSocket frame, send the frame followed by the data.
 * @param [in] data The data to send down the WebSocket.
 * @param [in] sendType The type of payload.  Either SEND_TYPE_TEXT or SEND_TYPE_BINARY.
 */
void WebsocketHandler::send(uint8_t* data, uint16_t length, uint8_t sendType) {
  HTTPS_LOGD(">> Websocket.send(): length=%d", length);
  WebsocketFrame frame;
  frame.fin    = 1;
  frame.rsv1   = 0;
  frame.rsv2   = 0;
  frame.rsv3   = 0;
  frame.opCode = sendType==SEND_TYPE_TEXT?OPCODE_TEXT:OPCODE_BINARY;
  frame.mask   = 0;
  if (length < 126) {
    frame.len = length;
    _con->writeBuffer((uint8_t *)&frame, sizeof(frame));
  } else {
    frame.len = 126;
    _con->writeBuffer((uint8_t *)&frame, sizeof(frame));
    uint16_t net_len = htons(length);
    _con->writeBuffer((uint8_t *)&net_len, sizeof(uint16_t));  // Convert to network byte order from host byte order
  }
  _con->writeBuffer(data, length);
  HTTPS_LOGD("<< Websocket.send()");
}  // Websocket::send

/**
 * Returns true if the connection has been closed, either by client or server
 */
bool WebsocketHandler::closed() {
  return _receivedClose || _sentClose;
}

}
