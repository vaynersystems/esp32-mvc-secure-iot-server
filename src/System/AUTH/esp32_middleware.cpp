
#include "esp32_middleware.h"

#include "../CORE/esp32_server.h"
#include <string_extensions.h>
extern esp32_server server;
#define STR_LINES PROGMEM "--------------------------------------------------------------------"
/**
 * The following middleware function is one of two functions dealing with access control. The
 * middlewareAuthentication() will interpret the HTTP Basic Auth header, check usernames and password,
 * and if they are valid, set the X-USERNAME and X-GROUP header.
 *
 * If they are invalid, the X-USERNAME and X-GROUP header will be unset. This is important because
 * otherwise the client may manipulate those internal headers.
 *
 * Having that done, further middleware functions and the request handler functions will be able to just
 * use req->getHeader("X-USERNAME") to find out if the user is logged in correctly.
 *
 * Furthermore, if the user supplies credentials and they are invalid, he will receive an 401 response
 * without any other functions being called.
 */
void esp32_middleware::middlewareAuthentication(HTTPRequest* req, HTTPResponse* res, std::function<void()> next) {
    // Unset both headers to discard any value from the client
    // This prevents authentication bypass by a client that just sets X-USERNAME   
    req->setHeader(HEADER_USERNAME, "");
    req->setHeader(HEADER_GROUP, "");

    bool jwtTokenValid = false;
    String jwtTokenFromCookie = req->getHeader("Cookie").c_str();
    if(jwtTokenFromCookie.length() > 7 && jwtTokenFromCookie.startsWith("Bearer "))
        jwtTokenFromCookie = jwtTokenFromCookie.substring(7);

    String jwtToken, jwtTokenFromRequest = req->getBearerAuthToken().c_str();
    String jwtDecodedString, jwtTokenPayload, jwtTokenPayloadVerify;
    std::string reqUsername;
    std::string reqPassword;


    //Serial.printf("Middlware authentication with jwt [%s]\n", jwtTokenFromRequest.c_str());
    
    if (req->getRequestString().substr(0, 7) == "/logout") {
        jwtTokenFromRequest.clear();
        res->setHeader("Cookie","expires=Thu, 01 Jan 1970 00:00:00 GMT;");
        req->setHeader(HEADER_AUTH, "");
        res->setHeader(HEADER_GROUP, "");
        next();
        //return;
    }
    else if (req->getRequestString().substr(0, 6) == "/login" && req->getMethod().c_str() == "POST") {
        jwtTokenFromRequest.clear();
        req->setHeader(HEADER_AUTH, "");
        res->setHeader(HEADER_GROUP, "");
        if (req->getMethod() == "POST") {
            int reqLength = req->getContentLength();
            //check user/pass.. issue token
            char buffer[256];
            reqLength = reqLength < sizeof(buffer) ? reqLength : sizeof(buffer);
            req->readChars(buffer, reqLength);
            buffer[reqLength] = NULL;

            // Serial.printf("Read: %s from request\r\n", buffer);
            DynamicJsonDocument doc(sizeof(buffer) * 8);
            DeserializationError err = deserializeJson(doc, buffer);
            if (err.code() == err.Ok) {
                const char* uname = doc["user_id"];
                const char* password = doc["password"];
                reqUsername = uname;
                reqPassword = password;
            }
            else {
                //Serial.printf("Error %d deserializing json: %s\n", err.code(), err.c_str());
            }
        }

    }
    else //check auth
    {
        
        if(jwtTokenFromCookie.length() > 0 && server.middleware->jwtTokenizer->decodeJWT(jwtTokenFromCookie,jwtDecodedString)){
            jwtTokenValid = true;
            //Serial.printf("Decoded auth token %s received from Cookie", jwtTokenFromCookie.c_str());
        } else {
            //Serial.printf("Failed to decode auth token %s received from Cookie", jwtTokenFromCookie.c_str());
            reqUsername = req->getBasicAuthUser();
            reqPassword = req->getBasicAuthPassword();
            Serial.printf("Using %s and %s from basic auth headers", reqUsername.c_str(), reqPassword.c_str());
        }
    }

    if(!jwtTokenValid){ //fall back to processing from request if available
        jwtTokenValid = (jwtTokenFromRequest.length() > 0 && !server.middleware->jwtTokenizer->decodeJWT(jwtTokenFromRequest, jwtDecodedString));
    }
    
    if (!jwtTokenValid){          
        if(req->getRequestString().substr(0, 6) == "/login" && req->getMethod() == "POST") {

            // Get login information from request       
            if (reqUsername.length() > 0 && reqPassword.length() > 0) {
                Serial.println("Checking loging info..");
                // _Very_ simple hardcoded user database to check credentials and assign the group
                bool authValid = true;
                std::string group = "";
                if (reqUsername == "admin" && reqPassword == "secret") {
                    group = "ADMIN";
                }
                else if (reqUsername == "user" && reqPassword == "test") {
                    group = "USER";
                }
                else {
                    authValid = false;
                }

                // If authentication was successful issue JWT token
                if (authValid) {
                    DynamicJsonDocument doc(128);
                    doc["user"] = reqUsername;
                    doc["password"] = reqPassword;
                    doc["role"] = group;
                    jwtTokenPayload.clear();
                    serializeJson(doc, jwtTokenPayload);

                    jwtToken = "Bearer ";
                    jwtToken += String(server.middleware->jwtTokenizer->encodeJWT(jwtTokenPayload));

                    //Serial.print("Token Payload: "); Serial.println(jwtTokenPayload);

                    std::string token = std::string(jwtToken.c_str());
                    //Serial.println(STR_LINES);
                    //Serial.print("Token: "); Serial.println(token.c_str());
                    // set custom headers and delegate control
                    req->setHeader(HEADER_USERNAME, reqUsername);
                    req->setHeader(HEADER_GROUP, group);
                    req->setHeader(HEADER_AUTH, token);
                    //verify
                    String strippedToken = String(token.substr(7).c_str());
                    if (!server.middleware->jwtTokenizer->decodeJWT(strippedToken, jwtTokenPayloadVerify)) {
                        std::string message = "[ERROR]  *** Failed to encode JWT token. Security validation Failed.";
                        res->print(message.c_str());
                    }
                    else {

                        //set response AUTH header
                        req->setHeader(HEADER_AUTH, token);

                        // The user tried to authenticate and was successful
                        // -> We proceed with this request.
                        jwtToken = String();
                        DynamicJsonDocument tokenDoc(384);
                        tokenDoc["access_token"] = token;
                        serializeJson(tokenDoc, jwtToken);

                        res->print(jwtToken);
                    }
                }
                else {
                    res->setStatusCode(401);
                    res->error();
                }
            }
            else {
                res->setStatusText("Unauthorized");
                res->setStatusCode(401);
                res->setHeader("Content-Type", "text/plain");
                res->error();
                //next();
            }
        }
        else{
            //not login page and invalid token.
            // if requesting private page, redirect to login
        }
    }
    else {
        //valid JWT Token.. read it
        DynamicJsonDocument doc(jwtDecodedString.length() * 4);
        DeserializationError err = deserializeJson(doc, jwtDecodedString);
        if (err.code() == DeserializationError::Ok) {
            #ifdef DEBUG
            if (doc["user"] == "admin") {
                Serial.printf("Login from admin\n");
            }
            const char* usr = doc["user"].as<const char*>();
            const char* pass = doc["password"].as<const char*>();
            const char* role = doc["role"].as<const char*>();
            
            //Serial.printf("Login from user %s with role %s\n", usr, role);
            #endif      
            //res->setHeader("WWW-Authenticate", jwtTokenFromRequest.c_str());
            //next();
        }
        else {
            // Failed to deserialize token
#ifdef DEBUG
            Serial.printf("Improperly Formed Token [%s]\n", jwtDecodedString.c_str());
#endif
            res->setHeader("WWW-Authenticate", "Basic realm=\"Internal\"");
        }
    }

    next();
}

/**
 * This function plays together with the middlewareAuthentication(). While the first function checks the
 * username/password combination and stores it in the request, this function makes use of this information
 * to allow or deny access.
 *
 * This example only prevents unauthorized access to every ResourceNode stored under an /internal/... path.
 */
void esp32_middleware::middlewareAuthorization(HTTPRequest* req, HTTPResponse* res, std::function<void()> next) {
    // Get the username (if any)
    std::string username = req->getHeader(HEADER_USERNAME);
    std::string authHeader = req->getHeader(HEADER_AUTH);
    String reqPath = String(req->getRequestString().c_str());
    String jwtTokenFromCookie = req->getHeader("Cookie").c_str();
    bool decodeResult = false;

    if(jwtTokenFromCookie.length() > 7 && jwtTokenFromCookie.startsWith("Bearer "))
        jwtTokenFromCookie = jwtTokenFromCookie.substring(7);


    String jwtTokenFromRequest = String(authHeader.c_str());
    if (jwtTokenFromRequest.length() > 7) {
        jwtTokenFromRequest.remove(0, 7);
        //Serial.print("Parsing JWT Token: "); Serial.println(jwtTokenFromRequest);
    }
    if(reqPath.equalsIgnoreCase("/logout")){
        //redirect to login
        res->setStatusCode(303);
        res->setHeader("Location","/logout.html");
        next();
        return;
    }
    
    String jwtDecodedString; //get from cookie
    if(jwtTokenFromCookie.length() > 0 && server.middleware->jwtTokenizer->decodeJWT(jwtTokenFromCookie,jwtDecodedString)){
        decodeResult= true;
    } else //otherwise try from request
        decodeResult = jwtTokenFromRequest.length() > 48 && server.middleware->jwtTokenizer->decodeJWT(jwtTokenFromRequest, jwtDecodedString);

    
    Serial.printf("**Authorization**\t Decoded: %s\n", decodeResult ? "Yes" : "No");
    if (decodeResult) {        
        DynamicJsonDocument doc(jwtDecodedString.length() * 2);
        DeserializationError err = deserializeJson(doc, jwtDecodedString);
        if (err.code() == err.Ok)
        {
            //Serial.print("User: "); Serial.println(doc["user"].as<const char*>());
            req->setHeader(HEADER_USERNAME, doc["user"].as<const char*>());
            req->setHeader(HEADER_GROUP, doc["role"].as<const char*>());
            next();
            return;
        }
        Serial.printf("ERROR OCCURED DESERIALIZING JWT TOKEN: %s\n", jwtDecodedString.c_str());
        
    }
    // Everything else will be allowed, so we call next()
    if(denyIfNotPublic(req,res)) return;
    if(denyIfNotAuthorized(req,res)) return;
    Serial.printf("Request for resource %s authorized.\n", req->getRequestString().c_str());
    next();
}

bool esp32_middleware::denyIfNotPublic(HTTPRequest* req, HTTPResponse* res){
    bool exclusion = false;
    string request = req->getRequestString();
    if(request.substr(0, 6) == "/login") exclusion = true;
    else if(request.substr(0, 7) == "/logout") exclusion = true;
    else if(request.substr(0, 11) == "/index.html") exclusion = true;
    else if(request.substr(0, 11) == "/login.html") exclusion = true;
    else if(request.substr(0, 12) == "/logout.html") exclusion = true;
    else if (request.substr(0, strlen("/CSS/style.css")) == "/CSS/style.css") exclusion = true;
    else if (request.substr(0, strlen("/JS/auth.js")) == "/JS/auth.js") exclusion = true;
    else if (request.substr(0, strlen("/favicon.ico")) == "/favicon.ico") exclusion = true;
    else if (request.substr(0, strlen("/JS/esp32_home.js")) == "/JS/esp32_home.js") exclusion = true;
     
   
    if(!exclusion){            
        Serial.printf("Request for resource %s rejected. Resource is not public.", request.c_str());
        if(ends_with(request,".gz") || ends_with(request, ".js") || ends_with(request, ".css"))
            return true;
        //if not resource file, redirect user
        res->setStatusCode(303);
        res->setHeader("Location", "/login?to=" + request);
        res->setHeader("WWW-Authenticate", "Basic realm=\"Internal\"");
        return true;
    }
    return false;
}

bool esp32_middleware::denyIfNotAuthorized(HTTPRequest* req, HTTPResponse* res){
    bool isInternalPath = req->getRequestString().substr(0, 5) == "/INT/";
    if(isInternalPath && req->getHeader(HEADER_GROUP) != "ADMIN"){
        Serial.printf("Request for resource %s unauthorized for user %u.", req->getRequestString().c_str(), req->getHeader(HEADER_USERNAME));
        res->setStatusCode(303);
        res->setHeader("Location", req->getHeader("Origin"));
        res->setHeader("WWW-Authenticate", "Basic realm=\"Internal\"");
        return true;
    }
    return false;
}

void esp32_middleware::middlewareSetTokenizer(char* pkData) {
    jwtTokenizer = new ArduinoJWT(pkData);
}