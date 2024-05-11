#ifndef _ESP32_SERVER_H
#define _ESP32_SERVER_H

#include "SPIFFS.h"
#include "../Config.h"

#include "System/AUTH/cert.h"
#include "System/AUTH/key.h"
#include "System/AUTH/ArduinoJWT.h"
#include "System/ROUTER/esp32_router.h"

#include "System/AUTH/esp32_middleware.h"

#include <HTTPSServer.hpp>
//#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <ArduinoJson.h>
#include <HTTPBodyParser.hpp>
#include <HTTPMultipartBodyParser.hpp>
#include <HTTPURLEncodedBodyParser.hpp>
#include "esp32_socket.h"
#include "esp32_config.h"

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
    
    bool registerNewCert(SSLCert* cert);
    esp32_middleware* middleware;  

    void registerNode(HTTPNode *node);

private:

    bool hasPrivateKey();
    bool hasPublicCert();

    void loadCertificates();
    void generateCert(
        const char* deviceName = "myesp32.local",
        const char* companyName = "Fancy Co",
        const char* validFrom = "20240101000000",
        const char* validTo = "20350101000000"
    );

    // Create an SSL certificate object from the files included above    
    esp32_router* _router;
    SSLCert* _cert;
    hw_timer_t* timer = NULL;
    bool _enableSSL;

    char *publicKey;
    char *privateKey;
};
#endif

