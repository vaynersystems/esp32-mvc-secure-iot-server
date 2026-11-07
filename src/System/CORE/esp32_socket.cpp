#include "esp32_socket.hpp"
const char *className = "esp32_socket";
static esp32_socket* activeClients[SOCKET_MAX];
// This method is called by the webserver to instantiate a new handler for each
// client that connects to the websocket endpoint
WebsocketHandler* esp32_socket::createSocket(){
      
    for(int i = 0; i < SOCKET_MAX; i++) {
        if (activeClients[i] == nullptr) {
            esp32_socket * handler = new esp32_socket(i+1);
            activeClients[i] = handler;
            #ifdef DEBUG_SOCKET
            Serial.printf("[%s] Creating new socket for client id %d\n", className, i+1);  
            #endif
            return handler;
        break;
        }
    }
    return NULL;
}

// void esp32_socket::setServiceName(string name){
//     service = name.c_str();
// }

// This method is called when a message arrives
void esp32_socket::onMessage(WebsocketInputStreambuf * input){
    //timer
    unsigned long startTime = millis();
    // Read the message with a size limit: the page only sends small service requests.
    static const size_t MAX_MESSAGE = 4096;
    std::istream in(input);
    string msg;
    char chunk[256];
    while (in.read(chunk, sizeof(chunk)) || in.gcount() > 0) {
        msg.append(chunk, in.gcount());
        if (msg.size() > MAX_MESSAGE) {
            close(); // message too big
            return;
        }
    }
    StaticJsonDocument<2048> message;
    if (deserializeJson(message, msg) != DeserializationError::Ok) return;

    // Only string fields: as<const char*>() is NULL for numbers/objects, and building a
    // std::string from NULL crashed the device.
    const char* service = message["service"] | (const char*)nullptr;
    const char* params = message["message"] | (const char*)nullptr;
    if (service == nullptr || params == nullptr) return; // not a service request: ignore
    // (Other messages used to be relayed to every connected client, letting any sender push
    // content into other users' pages.)

    esp32_service_route route;
    route.service = service;
    route.params = params;
    sendToClient(this->_clientId, esp32_router::handleServiceRequest(route));
    #ifdef DEBUG_SOCKET
    Serial.printf("[%s] Client [%d]. Processed websocket message in %d ms\n", className, this->_clientId, millis() - startTime);
    #endif
}

esp32_socket::~esp32_socket(){
    for(int i = 0; i < SOCKET_MAX; i++) {
        if (activeClients[i] == this) {
            activeClients[i] = nullptr;
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

void esp32_socket::sendToAllClients(string message){
    //Serial.printf("[%s] Sending message to all clients: %s\n", className, message.c_str());
    for(int i = 0; i < SOCKET_MAX; i++) {
        if (activeClients[i] != nullptr) {
            activeClients[i]->send(message, SEND_TYPE_TEXT);
        }
    }
}

void esp32_socket::sendToClient(int clientId, string message)
{
    for(int i = 0; i < SOCKET_MAX; i++) {
        if (activeClients[i] != nullptr && activeClients[i]->_clientId == clientId) {
            activeClients[i]->send(message, SEND_TYPE_TEXT);
        }
    }
}
