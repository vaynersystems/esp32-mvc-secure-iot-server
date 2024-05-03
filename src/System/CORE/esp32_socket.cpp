#include "esp32_socket.h"

// This method is called by the webserver to instantiate a new handler for each
// client that connects to the websocket endpoint
WebsocketHandler* esp32_socket::createSocket(){
    Serial.println("Creating new socket!");
    esp32_socket * handler = new esp32_socket();
    for(int i = 0; i < SOCKET_MAX; i++) {
        if (activeClients[i] == nullptr) {
            activeClients[i] = handler;
        break;
        }
    }
    return handler;
}

// This method is called when a message arrives
void esp32_socket::onMessage(WebsocketInputStreambuf * input){
    // Get the input message
    std::ostringstream ss;
    std::string msg;
    ss << input;
    msg = ss.str();

    // Send it back to every client
    for(int i = 0; i < SOCKET_MAX; i++) {
        if (activeClients[i] != nullptr) {
            activeClients[i]->send(msg, SEND_TYPE_TEXT);
        }
    }
}

// Handler function on connection close
void esp32_socket::onClose(){
    for(int i = 0; i < SOCKET_MAX; i++) {
        if (activeClients[i] == this) {
            activeClients[i] = nullptr;
        }
    }
}

    

