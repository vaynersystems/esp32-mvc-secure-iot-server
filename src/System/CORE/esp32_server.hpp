#ifndef _ESP32_SERVER_H
#define _ESP32_SERVER_H

#include "SPIFFS.h"
#include "../Config.h"

#include "System/AUTH/ArduinoJWT.h"
#include "System/ROUTER/esp32_router.h"
#include "System/AUTH/esp32_middleware.hpp"

#include <HTTPSServer.hpp>
//#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <ArduinoJson.h>
#include <HTTPBodyParser.hpp>
#include <HTTPMultipartBodyParser.hpp>
#include <HTTPURLEncodedBodyParser.hpp>
#include "esp32_socket.hpp"
#include "esp32_config.hpp"
#include <System/AUTH/CERT/esp32_cert_nvs.hpp>
#include <System/AUTH/CERT/esp32_cert_spiffs.hpp>

using namespace httpsserver;


class esp32_server
{
public:
    
    esp32_server();
    //esp32_server(SSLCert * cert);
    bool start();
    bool stop();
    bool isRunning();
    void step();

    HTTPSServer *secureServer;
    HTTPServer *unsecureServer;
    
    SSLCert * getCertificate();
    bool importCertFromTemporaryStorage();
    void generateCertificate(const char * deviceName, const char* companyName, const char* validFrom, const char* validTo);
    esp32_middleware* middleware;  

    void registerNode(HTTPNode *node);

private:

    // Create an SSL certificate object from the files included above    
    esp32_router* _router;
    //esp32_cert_nvs* _certManager;
    esp32_cert_base* _certManager;
    uint32_t _restartMillis = 0;
//    hw_timer_t* timer = NULL;
    bool _enableSSL;
};
#endif

