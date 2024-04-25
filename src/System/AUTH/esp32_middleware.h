#ifndef _ESP32_MIDDLEWARE_H
#define _ESP32_MIDDLEWARE_H




#include "../Config.h"
#include <HTTPSServer.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <ArduinoJson.h>
#include "ArduinoJWT.h"
#include <HTTPBodyParser.hpp>
#include <HTTPMultipartBodyParser.hpp>
#include <HTTPURLEncodedBodyParser.hpp>
using namespace httpsserver;




class esp32_middleware
{
public:

	static void middlewareAuthentication(HTTPRequest* req, HTTPResponse* res, std::function<void()> next);
    static void middlewareAuthorization(HTTPRequest* req, HTTPResponse* res, std::function<void()> next);
    void middlewareSetTokenizer(char* pkData);
   
private:

    //for token based access (instead of basic)
    
    ArduinoJWT* jwtTokenizer;
    //Get JWT Token
    //String GetJwtTokenFromHeader(HTTPRequest* request);

    static bool denyIfNotPublic(HTTPRequest* req, HTTPResponse* res);
    static bool denyIfNotAuthorized(HTTPRequest* req, HTTPResponse* res);
};

#endif