#include "esp32_socket.h"
static esp32_socket* activeClients[SOCKET_MAX];
// This method is called by the webserver to instantiate a new handler for each
// client that connects to the websocket endpoint
WebsocketHandler* esp32_socket::createSocket(){
    Serial.println("Creating new socket!");    
    for(int i = 0; i < SOCKET_MAX; i++) {
        if (activeClients[i] == nullptr) {
            esp32_socket * handler = new esp32_socket(i);
            activeClients[i] = handler;
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
    // Get the input message
    ostringstream ss;
    string msg;
    ss << input;
    msg = ss.str();    
    //Serial.printf("Received web socket message %s\n", msg.c_str());    
    StaticJsonDocument<2048> message;
    deserializeJson(message, msg);

    
    esp32_service_route route;
    if(!message["service"].isNull() && !message["message"].isNull()){        
        route.service = message["service"].as<const char*>();
        route.params = message["message"].as<const char *>();
        sendToClient(this->_clientId, esp32_router::handleServiceRequest(route));
        //sendToAllClients(esp32_router::handleServiceRequest(route));
        
    } 
    else 
        sendToAllClients(msg);
    #ifdef DEBUG
    Serial.printf("Processed websocket message in %d ms\n", millis() - startTime);
    #endif
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
    //Serial.printf("Sending message to all clients: %s\n", message.c_str());
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
