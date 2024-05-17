#ifndef _ESP32_MIDDLEWARE_H
#define _ESP32_MIDDLEWARE_H




#include "../Config.h"
#include <HTTPSServer.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <ArduinoJson.h>
#include "ArduinoJWT.h"
#include <system_helper.h>
#include <HTTPBodyParser.hpp>
#include <HTTPMultipartBodyParser.hpp>
#include <HTTPURLEncodedBodyParser.hpp>
#include <System/MODULES/LOGGING/esp32_logging.hpp>
using namespace httpsserver;

extern esp32_logging logger;


class esp32_middleware
{
public:

	static void middlewareAuthentication(HTTPRequest* req, HTTPResponse* res, std::function<void()> next);
    static void middlewareAuthorization(HTTPRequest* req, HTTPResponse* res, std::function<void()> next);
    void middlewareSetTokenizer(char* pkData);
    int initPublicPages();
   
private:

    //for token based access (instead of basic)
    
    ArduinoJWT* jwtTokenizer;
    //Get JWT Token
    //String GetJwtTokenFromHeader(HTTPRequest* request);

    static bool denyIfNotPublic(HTTPRequest* req, HTTPResponse* res);
    static bool denyIfNotAuthorized(HTTPRequest* req, HTTPResponse* res);
    static void setAuthHeaders(HTTPRequest* req, const char * user, const char * password, const char * token);

    bool isPublicPage(string path);

    vector<string> _publicPages;
};

#endif