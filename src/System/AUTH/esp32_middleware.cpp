
#include "esp32_middleware.h"
#include "esp32_authentication.h"
#include "../CORE/esp32_server.h"
#include <string_extensions.h>
#include <esp_task_wdt.h>
#include <BLEEddystoneTLM.h>
extern esp32_server server;
#define STR_LINES PROGMEM "--------------------------------------------------------------------"

void esp32_middleware::middlewareAuthentication(HTTPRequest* req, HTTPResponse* res, std::function<void()> next) {
    // Unset both headers to discard any value from the client
    // This prevents authentication bypass by a client that just sets X-USERNAME   
    setAuthHeaders(req,"","","");

    bool jwtTokenValid = false;
    String jwtTokenFromCookie = req->getHeader("Cookie").c_str();
    if(jwtTokenFromCookie.length() > 7 && jwtTokenFromCookie.startsWith("Bearer "))
        jwtTokenFromCookie = jwtTokenFromCookie.substring(7);

    String jwtToken, jwtTokenFromRequest = req->getBearerAuthToken().c_str();
    String jwtDecodedString, jwtTokenPayload, jwtTokenPayloadVerify;
    std::string reqUsername;
    std::string reqPassword;
    bool isLoginPage = strstr(req->getRequestString().substr(0, 6).c_str(), "/login") != nullptr ;
    bool isPostRequest = strstr(req->getMethod().c_str(), "POST") != nullptr ;


    
    if (req->getRequestString().substr(0, 7) == "/logout") {
        jwtTokenFromRequest.clear();
        res->setHeader("Set-Cookie","expires=Thu, 01 Jan 1970 00:00:00 GMT;auth=");
        req->setHeader(HEADER_AUTH, "");
        res->setHeader(HEADER_GROUP, "");
        next();
        //return;
    }
    else if (isLoginPage && isPostRequest) {
        jwtTokenFromRequest.clear();
        req->setHeader(HEADER_AUTH, "");
        res->setHeader(HEADER_GROUP, "");
        if (isPostRequest) {
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
            //Serial.printf("%s request to url: %s. Using %s and %s from basic auth headers | isLoginPage: %s, isPostRequest: %s", req->getMethod().c_str(), req->getRequestString().c_str(), reqUsername.c_str(), reqPassword.c_str(), isLoginPage ? "True" : "False", isPostRequest ? "True" : "False");
        }
    }

    if(!jwtTokenValid){ //fall back to processing from request if available
        jwtTokenValid = (jwtTokenFromRequest.length() > 0 && !server.middleware->jwtTokenizer->decodeJWT(jwtTokenFromRequest, jwtDecodedString));
    }
    
    if (!jwtTokenValid){          
        if(isLoginPage && isPostRequest) {

            // Get login information from request       
            if (reqUsername.length() > 0 && reqPassword.length() > 0) {
                //Serial.println("Checking loging info..");
                esp32_user_auth_info info = esp32_authentication::authenticateUser(reqUsername.c_str(), reqPassword.c_str());
                //Serial.printf("Authentication for: %s : %s\n", info.username.c_str(), info.authenticated ? "VALID" : "INVALID");
                
                // if authentication was successful issue JWT token
                if (info.authenticated) {
                    DynamicJsonDocument doc(128);
                    doc["user"] = reqUsername;
                    doc["password"] = reqPassword;
                    doc["role"] = info.role.c_str();
                    doc["exp"] = getTime() +( 3600 * 24 * 7);
                    jwtTokenPayload.clear();
                    serializeJson(doc, jwtTokenPayload);

                    jwtToken = "Bearer ";
                    jwtToken += String(server.middleware->jwtTokenizer->encodeJWT(jwtTokenPayload));
                    
                    setAuthHeaders(req, reqUsername.c_str(), info.role.c_str(),jwtToken.c_str());

                    std::string token = std::string(jwtToken.c_str());
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
                        res->print(jwtToken.c_str());
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

            if(req->getRequestString().substr(0, 6) == "/login" && req->getMethod() == "POST") {
                //redirect
                // auto returnUrl = req->getHeader("X_RETURN_URL");
                // if(returnUrl.length() == 0)
                // returnUrl = "index.html";
                // res->setStatusCode(303);
                
                // res->setHeader("Location",returnUrl);
                // return;
                res->print(jwtToken);     
                //return;
            }   

            if(doc["exp"].as<unsigned long>() < getTime()){
                res->setStatusCode(401);

            } 

            #ifdef DEBUG
            if (doc["user"] == "admin") {
                Serial.printf("Login from admin\n");
            }
            const char* usr = doc["user"].as<const char*>();
            const char* pass = doc["password"].as<const char*>();
            const char* role = doc["role"].as<const char*>();
            
            //Serial.printf("Login from user %s with role %s\n", usr, role);
            #endif 
            setAuthHeaders(req, "","",jwtToken.c_str());
        }
        else {
            // Failed to deserialize token
#ifdef DEBUG
            Serial.printf("Improperly Formed Token [%s]\n", jwtDecodedString.c_str());
#endif
            //Should take user to login page
            res->setHeader("WWW-Authenticate", "Basic realm=\"Internal\"");
        }
    }

    next();
}

/**
 * This function plays together with the middlewareAuthentication(). While the first function checks the
 * username/password combination and stores it in the request, this function makes use of this information
 * to allow or deny access.
 */
void esp32_middleware::middlewareAuthorization(HTTPRequest* req, HTTPResponse* res, std::function<void()> next) {
    // Get the username (if any)
    string username = req->getHeader(HEADER_USERNAME);
    string authHeader = req->getHeader(HEADER_AUTH);
    string cookieHeader = req->getHeader(HEADER_COOKIE).c_str();
    string jwtTokenFromCookie = "";
    String reqPath = String(req->getRequestString().c_str());
    
    bool decodeResult = false;
    bool isLoginPage = strstr(req->getRequestString().substr(0, 6).c_str(), "/login") != nullptr ;
    bool isPostRequest = strstr(req->getMethod().c_str(), "POST") != nullptr ;
    esp32_user_auth_info authResult;

    //check if cookie has auth
    vector<string> chunks = explode(cookieHeader,";");
    for(size_t idx = 0; idx < chunks.size(); idx++){
        vector<string> parts = explode(chunks[idx],"=");
        if(parts.size() == 1) //no name
            Serial.printf("Invalid cookie parsed without name %s\n", parts[0].c_str());
        else if(strcmp(parts[0].c_str(),"auth") == 0){
            jwtTokenFromCookie = parts[1];
            //Serial.printf("Found auth header %s in cookie\n", jwtTokenFromCookie.c_str());
        }
    }
       

    String jwtTokenFromRequest = String(authHeader.c_str());
    if (jwtTokenFromRequest.length() > 7) {
        jwtTokenFromRequest.remove(0, 7);
        //Serial.print("Parsing JWT Token from header: "); Serial.println(jwtTokenFromRequest);
    }

    //if an auth header is passed, it gets priority over cookie
    String jwtToken = jwtTokenFromRequest.length() > 0 ? jwtTokenFromRequest : jwtTokenFromCookie.c_str() ;
    if(reqPath.equalsIgnoreCase("/logout")){
        //redirect to login
        //esp_task_wdt_reset();
        res->setStatusCode(303);
        res->setHeader("Location","/logout.html");
        next(); 
        return;
    }
    
    String jwtDecodedString; //get from cookie
    if(jwtToken.length() > 0 && server.middleware->jwtTokenizer->decodeJWT(jwtToken,jwtDecodedString))
        decodeResult= true;

    //Serial.printf("**Authorization**\t Decoded: %s\nJWT Encoded: %s\nJWT Decoded: %s\n", decodeResult ? "Yes" : "No", jwtToken.c_str(), jwtDecodedString.c_str());
    if (decodeResult) {        
        DynamicJsonDocument doc(jwtDecodedString.length() * 2);
        DeserializationError err = deserializeJson(doc, jwtDecodedString);
        if (err.code() == err.Ok)
        {
            //TODO: create config setting to force verify token
            //verify token is valid    
            authResult = esp32_authentication::authenticateUser(doc["user"].as<const char*>(),doc["password"].as<const char*>());
            //authResult.authenticated = true;
            // Serial.printf(
            //     "**Authorization** Result %s\nUsername: %s\t Password: %s\tRole: %s\n", 
            //     authResult.authenticated ? "Authenticated" : "Unauthorized", 
            //     authResult.username.c_str(), 
            //     authResult.password.c_str(), 
            //     authResult.role.c_str()
            // ); 
            if(authResult.authenticated){                
            
                setAuthHeaders(req, doc["user"].as<const char*>(), doc["role"].as<const char*>(),jwtToken.c_str());            
                if(isLoginPage && isPostRequest) //special case for posting login info
                    return;        
            }
        }
        else Serial.printf("ERROR [%i] OCCURED DESERIALIZING JWT TOKEN: %s\n Details: %s", jwtDecodedString.c_str(), err.code(), err.c_str());
        
    }
    // Everything else will be allowed, so we call next()
    if(!authResult.authenticated && denyIfNotPublic(req,res)) return;
    if(!authResult.authenticated && denyIfNotAuthorized(req,res)) return;
    //Serial.printf("Request for resource %s authorized.\n", req->getRequestString().c_str());
    next();
}

bool esp32_middleware::denyIfNotPublic(HTTPRequest* req, HTTPResponse* res){
    bool exclusion = false;
    string request = req->getRequestString();
    //if(strstr(request.c_str(),"/") != nullptr) exclusion = true; //EXCLUDE EVERYTHING, making all public
    if(request.substr(0, 6) == "/login") exclusion = true;
    else if(request.substr(0, strlen("/logout")) == "/logout") exclusion = true;
    else if(request.substr(0, strlen("/index.html")) == "/index.html") exclusion = true;
    else if(request.substr(0, strlen("/login.html")) == "/login.html") exclusion = true;
    else if(request.substr(0, strlen("/logout.html")) == "/logout.html") exclusion = true;
    else if(request.substr(0, strlen("/CSS/style.css")) == "/CSS/style.css") exclusion = true;
    else if(request.substr(0, strlen("/JS/auth.js")) == "/JS/auth.js") exclusion = true;
    else if(request.substr(0, strlen("/favicon.ico")) == "/favicon.ico") exclusion = true;
    else if(request.substr(0, strlen("/T/V/_footer.html")) == "/T/V/_footer.html") exclusion = true;
    else if(request.substr(0, strlen("/esp32_home")) == "/esp32_home") exclusion = true;
    else if(request.substr(0, strlen("/JS/esp32_home.js")) == "/JS/esp32_home.js") exclusion = true;
     
   
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
    bool isInternalPath = req->getRequestString().substr(0, 5) == INTERNAL_ROOT;
    if(isInternalPath && req->getHeader(HEADER_GROUP) != "ADMIN"){
        Serial.printf("Request for resource %s unauthorized for user %u.", req->getRequestString().c_str(), req->getHeader(HEADER_USERNAME));
        res->setStatusCode(401);
        res->setHeader("Location", req->getHeader("Origin"));
        res->setHeader("WWW-Authenticate", "Basic realm=\"Internal\"");
        return true;
    }
    return false;
}

void esp32_middleware::middlewareSetTokenizer(char* pkData) {
    jwtTokenizer = new ArduinoJWT(pkData);
}

void esp32_middleware::setAuthHeaders(HTTPRequest* req, const char * user, const char * group, const char * token){
    req->setHeader(HEADER_USERNAME, user);
    req->setHeader(HEADER_GROUP, group);
    req->setHeader(HEADER_AUTH, token);
}