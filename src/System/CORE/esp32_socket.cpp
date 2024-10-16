#include "esp32_socket.h"
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
    // Get the input message
    ostringstream ss;
    string msg;
    ss << input;
    msg = ss.str();    
    //Serial.printf("[%s] Received web socket message %s\n", className, msg.c_str());    
    StaticJsonDocument<2048> message;
    deserializeJson(message, msg);

    
    esp32_service_route route;
    if(!message["service"].isNull() && !message["message"].isNull()){        
        route.service = message["service"].as<const char*>();
        route.params = message["message"].as<const char *>();
        sendToClient(this->_clientId, esp32_router::handleServiceRequest(route));
        //sendToAllClients(esp32_router::handleServiceRequest(route));
        #ifdef DEBUG_SOCKET
        Serial.printf("[%s] Client [%d]. Processed websocket message in %d ms\n", className, this->_clientId, millis() - startTime);
        #endif
        
    } 
    else {
        sendToAllClients(msg);
        #ifdef DEBUG_SOCKET
        Serial.printf("[%s]All clientsProcessed websocket message in %d ms\n", className, millis() - startTime);
        #endif
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
