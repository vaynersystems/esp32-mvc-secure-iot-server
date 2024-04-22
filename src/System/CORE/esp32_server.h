#ifndef _ESP32_SERVER_H
#define _ESP32_SERVER_H



#include "../Config.h"



#include "../AUTH/cert.h"
#include "../AUTH/key.h"
#include "../AUTH/ArduinoJWT.h"
#include "../ROUTER/esp32_router.h"

#include "../AUTH/esp32_middleware.h"


#include <HTTPSServer.hpp>
//#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <ArduinoJson.h>
#include <HTTPBodyParser.hpp>
#include <HTTPMultipartBodyParser.hpp>
#include <HTTPURLEncodedBodyParser.hpp>

using namespace httpsserver;


class esp32_server
{
public:
    
    esp32_server(SSLCert * cert);
    bool start();
    bool stop();
    bool isRunning();

    void step();
    // Create an SSL-enabled server that uses the certificate
    // The contstructor takes some more parameters, but we go for default values here.
    HTTPSServer *secureServer;
    HTTPServer *unsecureServer;
    
    
    

    void DisplayLoginPage(HTTPResponse* res);
    void DisplayErrorPage(HTTPResponse* res, String errorMessage);
    bool RegisterNewCert(SSLCert* cert);
    esp32_middleware* middleware;

private:
    // Create an SSL certificate object from the files included above
    
    esp32_router* _router;
    SSLCert* _cert;
    hw_timer_t* timer = NULL;
};
#endif
