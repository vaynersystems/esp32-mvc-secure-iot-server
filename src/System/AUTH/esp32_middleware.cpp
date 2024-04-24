
#include "esp32_middleware.h"

#include "../CORE/esp32_server.h"
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
   // Serial.printf("\n\n\n");
    //Serial.println(STR_LINES);
   // Serial.printf("*  Processing request for:    %s\n", req->getRequestString().c_str());
    //Serial.println(STR_LINES);
    req->setHeader(HEADER_USERNAME, "");
    req->setHeader(HEADER_GROUP, "");
    bool jwtTokenValid = false;
    String jwtTokenFromCookie = req->getHeader("Cookie").c_str();
    if(jwtTokenFromCookie.length() > 7 && jwtTokenFromCookie.startsWith("Bearer "))
        jwtTokenFromCookie = jwtTokenFromCookie.substring(7);

    String jwtToken, jwtTokenFromRequest = req->getBearerAuthToken().c_str();
    String jwtDecodedString, jwtTokenPayload, jwtTokenPayloadVerify;

    //Serial.printf("Middlware authentication with jwt [%s]\n", jwtTokenFromRequest.c_str());

    std::string reqUsername;
    std::string reqPassword;

    if (req->getRequestString().substr(0, 7) == "/logout") {
        jwtTokenFromRequest.clear();
        res->setHeader("Cookie","expires=Thu, 01 Jan 1970 00:00:00 GMT;");
        req->setHeader(HEADER_AUTH, "");
        res->setHeader(HEADER_GROUP, "");
        //Serial.println("Logging out..");
        //server.DisplayErrorPage(res, "Logged out!");
        //server.DisplayLoginPage(res);
        next();
        //return;
    }
    else if (req->getRequestString().substr(0, 6) == "/login" && req->getMethod().c_str() == "POST") {
        jwtTokenFromRequest.clear();
        req->setHeader(HEADER_AUTH, "");
        res->setHeader(HEADER_GROUP, "");
        //Serial.println("Logging in..");
        // if (req->getMethod() == "GET")
        //     server.DisplayLoginPage(res);
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
                /* req->getParams()->getQueryParameter("user", reqUsername);
                 req->getParams()->getQueryParameter("password", reqPassword);*/
                //Serial.printf("Setting user and pass from post %s %s\n", reqUsername.c_str(), reqPassword.c_str());
            }
            else {
                //Serial.printf("Error %d deserializing json: %s\n", err.code(), err.c_str());
            }
        }

    }
    else //check auth
    {
        //Serial.printf("Checking auth token %s received from Cookie", jwtTokenFromCookie.c_str());
        if(jwtTokenFromCookie.length() > 0 && server.middleware->jwtTokenizer->decodeJWT(jwtTokenFromCookie,jwtDecodedString)){
            jwtTokenValid = true;
        } else {
            reqUsername = req->getBasicAuthUser();
            reqPassword = req->getBasicAuthPassword();
        }
    }

    if(!jwtTokenValid){ //fall back to processing from request if available
        jwtTokenValid = (jwtTokenFromRequest.length() > 0 && !server.middleware->jwtTokenizer->decodeJWT(jwtTokenFromRequest, jwtDecodedString));
    }
    
    if (!jwtTokenValid){          
        if(req->getRequestString().substr(0, 6) == "/login" && req->getMethod() == "POST") {

            //Serial.print("Read from Authorization header: "); Serial.println(jwtDecodedString.c_str());

            // Get login information from request
            // If you use HTTP Basic Auth, you can retrieve the values from the request.
            // The return values will be empty strings if the user did not provide any data,
            // or if the format of the Authorization header is invalid (eg. no Basic Method
            // for Authorization, or an invalid Base64 token)

            /*char* jwtToken = (char* )malloc(req->getHeader("Authoriztion").length());
            strcpy(jwtToken, req->getHeader("Authoriztion").c_str());*/

            // If the user entered login information, we will check it
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
                        //req->setHeader(AUTH_HEADER, token);
                        //res->setHeader(AUTH_HEADER, token);

                        // The user tried to authenticate and was successful
                        // -> We proceed with this request.
                        jwtToken = String();
                        DynamicJsonDocument tokenDoc(384);
                        tokenDoc["access_token"] = token;
                        serializeJson(tokenDoc, jwtToken);

                        res->print(jwtToken);
                        //handleRoot(req, res);
                    }
                }
                else {
                    server.DisplayErrorPage(res, "Failed to authenticate using this username and password!");
                }
            }
            else {
                //Serial.println("No log info passed..");
                // No attempt to authenticate
                // -> Let the request pass through by calling next()
                //res->setStatusText("Unauthorized");
                //res->setHeader("Content-Type", "text/plain");
                next();
            }
        }
        else{
            //not login page and invalid token.
            // if requesting private page, redirect to login
        }
    }
    else {
        //valid JWT Token.. read it
        //Serial.printf("Processing (jwt) [%s]\n", jwtDecodedString.c_str());
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
            //if(jwtDecodedString.length() <= 0){
#ifdef DEBUG
            Serial.printf("Improperly Formed Token [%s]\n", jwtDecodedString.c_str());
#endif
            //res->setStatusCode(401);
            //res->setStatusText("Unauthorized");
            //res->setHeader("Content-Type", "text/plain");

            // This should trigger the browser user/password dialog, and it will tell
            // the client how it can authenticate
            res->setHeader("WWW-Authenticate", "Basic realm=\"Internal\"");

            // Small error text on the response document. In a real-world scenario, you
            // shouldn't display the login information on this page, of course ;-)
            //res->println("401. Unauthorized [Invalid JWT Token] (try admin/secret or user/test)");
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
        res->setStatusCode(303);
        res->setHeader("Location","logout.html");
        next();
        return;
    }
    
    String jwtDecodedString; //get from cookie
    if(jwtTokenFromCookie.length() > 0 && server.middleware->jwtTokenizer->decodeJWT(jwtTokenFromCookie,jwtDecodedString)){
        decodeResult= true;
    } else //otherwise try from request
        decodeResult = jwtTokenFromRequest.length() > 48 && server.middleware->jwtTokenizer->decodeJWT(jwtTokenFromRequest, jwtDecodedString);

    bool isInternalPath = req->getRequestString().substr(0, 5) == "/INT/";

    //Serial.printf("**Authorization**\t Decoded: %s, Is Internal: %s\n", decodeResult ? "Yes" : "No", isInternalPath ? "Yes" : "No");
    if (!decodeResult
        && isInternalPath) {

        // Check that only logged-in users may get to the internal area (All URLs starting with /internal)
       
        res->setStatusCode(303);
        res->setHeader("Location", "login");
    }
    else if (decodeResult) {        
        DynamicJsonDocument doc(jwtDecodedString.length() * 2);
        //username = std::string(doc["user"].as<char*>());
        DeserializationError err = deserializeJson(doc, jwtDecodedString);
        if (err.code() == err.Ok)
        {
            Serial.print("User: "); Serial.println(doc["user"].as<const char*>());
            req->setHeader(HEADER_USERNAME, doc["user"].as<const char*>());
            req->setHeader(HEADER_GROUP, doc["role"].as<const char*>());
            next();
        }
        else
        {
            res->setStatusCode(303);
            res->setHeader("Location", "login");
        }
    }
    else // Everything else will be allowed, so we call next()
        next();
}


//Get JWT Token
String esp32_middleware::GetJwtToken(HTTPRequest* request) {
    std::string authstr = request->getHeader(HEADER_AUTH);
    // Get the length of the token
    size_t sourceLength = authstr.length();
    // Only handle basic auth tokens
    if (authstr.substr(0, 7) != "Bearer ") {
        return String();
    }
    // If the token is too long, skip
    if (sourceLength > 500) {
        return String();
    }
    else {
        // Try to decode. As we are using mbedtls anyway, we can use that function
        unsigned char* bufOut = new unsigned char[authstr.length()];

        size_t outputLength = 0;
        int res = mbedtls_base64_decode(
            bufOut,
            sourceLength,
            &outputLength,
            ((const unsigned char*)authstr.substr(7).c_str()), // Strip "Bearer "
            sourceLength - 7 // Strip "Basic "
        );
        // Failure of decoding
        if (res != 0) {
            delete[] bufOut;
            return String();
        }
        String tokenRes = String((char*)bufOut);
        delete[] bufOut;
        return tokenRes;

    }
}


void esp32_middleware::middlewareSetTokenizer(char* pkData) {
    jwtTokenizer = new ArduinoJWT(pkData);
}