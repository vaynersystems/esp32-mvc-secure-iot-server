#ifndef _ESP32_SOCKET_H
#define _ESP32_SOCKET_H

#include "../Config.h"
#include "../AUTH/ArduinoJWT.h"
#include "../ROUTER/esp32_router.h"

#include "../AUTH/esp32_middleware.hpp"

#include <HTTPSServer.hpp>
//#include <SSLCert.hpp>
#include <ArduinoJson.h>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <WebsocketHandler.hpp>

using namespace httpsserver;


class esp32_socket : public WebsocketHandler
{
public:
    esp32_socket(int clientId){
        _clientId = clientId;
    }
    
    // This method is called by the webserver to instantiate a new handler for each
    // client that connects to the websocket endpoint
    static WebsocketHandler* createSocket();

    //void setServiceName(string name);

    // This method is called when a message arrives
    void onMessage(WebsocketInputStreambuf * input);

    // Handler function on connection close
    void onClose();
    

    esp32_middleware* middleware;

protected:
    //hw_timer_t* timer = NULL;
    //string service;
    void sendToAllClients(string message);
    void sendToClient(int clientId, string message);

private:
    int _clientId = 0;
    
};


#endif

