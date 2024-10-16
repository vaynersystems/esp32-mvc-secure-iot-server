
#include "esp32_middleware.h"
#include "esp32_authentication.h"
#include "../CORE/esp32_server.h"
#include "string_helper.h"
#include <esp_task_wdt.h>
#include <BLEEddystoneTLM.h>
extern esp32_server server;
#define STR_LINES PROGMEM "--------------------------------------------------------------------"

void esp32_middleware::middlewareAuthentication(HTTPRequest* req, HTTPResponse* res, std::function<void()> next) {
    // Unset both headers to discard any value from the client
    // This prevents authentication bypass by a client that just sets X-USERNAME   
    setAuthHeaders(req,"","","");

    bool jwtTokenValid = false;
    string jwtTokenFromCookie = req->getHeader("Cookie").c_str();
    string jwtToken, jwtTokenFromRequest = req->getBearerAuthToken().c_str();
    string jwtDecodedString, jwtTokenPayload, jwtTokenPayloadVerify;
    string reqUsername;
    string reqPassword;
    bool isLoginPage = strstr(req->getRequestString().substr(0, 6).c_str(), "/login") != nullptr ;
    bool isPostRequest = strstr(req->getMethod().c_str(), "POST") != nullptr ;

    if (req->getRequestString().substr(0, 7) == "/logout") {
        jwtTokenFromRequest.clear();
        res->setHeader("Set-Cookie","expires=Thu, 01 Jan 1970 00:00:00 GMT;auth=; path=/");
        req->setHeader(HEADER_AUTH, "");
        res->setHeader(HEADER_GROUP, "");
        next();
        //return;
    }
    else if (isLoginPage && isPostRequest) {
        jwtTokenFromRequest.clear();
        req->setHeader(HEADER_AUTH, "");
        res->setHeader(HEADER_GROUP, "");
        
        int reqLength = req->getContentLength();
        //check user/pass.. issue token
        char buffer[256];
        reqLength = reqLength < sizeof(buffer) ? reqLength : sizeof(buffer);
        req->readChars(buffer, reqLength);
        buffer[reqLength] = 0;

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
            logger.logError(string_format("Middleware Authentication error %d deserializing json: %s", err.code(), err.c_str()));            
        }
        

    }
    else //check auth
    {   
        reqUsername = req->getBasicAuthUser();
        reqPassword = req->getBasicAuthPassword();
        //Serial.printf("%s request to url: %s. Using %s and %s from basic auth headers | isLoginPage: %s, isPostRequest: %s\n", req->getMethod().c_str(), req->getRequestString().c_str(), reqUsername.c_str(), reqPassword.c_str(), isLoginPage ? "True" : "False", isPostRequest ? "True" : "False");
    }

    if(!jwtTokenValid){ //fall back to processing from request if available
        
        jwtTokenValid = (jwtTokenFromRequest.length() > 0 && !server.middleware->jwtTokenizer->decodeJWT(jwtTokenFromRequest, jwtDecodedString));
    }
    
    if (!jwtTokenValid){          
        if(isLoginPage && isPostRequest) {

            // Get login information from request       
            if (reqUsername.length() > 0 && reqPassword.length() > 0 && reqUsername.length() <= 32 && reqPassword.length() <=32) {
                //Serial.println("Checking loging info..");
                esp32_user_auth_info info = esp32_authentication::authenticateUser(reqUsername.c_str(), reqPassword.c_str());
                string message = string_format("Authentication for: %s : %s", reqUsername.c_str(), info.authenticated ? "VALID" : "INVALID");
                logger.logInfo(message.c_str(), auth);
                Serial.println(message.c_str());
                
                
                // if authentication was successful issue JWT token
                if (info.authenticated) {
                    DynamicJsonDocument doc(512);
                    doc["user"] = info.username;
                    doc["role"] = info.role.c_str();
                    doc["password"] = info.password;
                    doc["exp"] = getTime() +( 3600 * 24 * 7);
                    jwtTokenPayload.clear();
                    serializeJson(doc, jwtTokenPayload);

                    res->print("{\"access_token\": \"Bearer ");
                    res->print(server.middleware->jwtTokenizer->encodeJWT(jwtTokenPayload).c_str());
                    res->print("\"}");
                    // jwtToken = "Bearer ";
                    // jwtToken = jwtToken + server.middleware->jwtTokenizer->encodeJWT(jwtTokenPayload);
                    
                    setAuthHeaders(req, reqUsername.c_str(), info.role.c_str(),jwtToken.c_str());
                   
                    // //verify
                    // string strippedToken = jwtToken.substr(7).c_str();
                    // if (!server.middleware->jwtTokenizer->decodeJWT(strippedToken, jwtTokenPayloadVerify)) {
                    //     logger.logInfo("[ERROR]  *** Failed to encode JWT token. Security validation Failed.", auth);
                    // }
                    // else {

                    //     //set response AUTH header
                    //     req->setHeader(HEADER_AUTH, jwtToken);
                    //     // The user tried to authenticate and was successful
                    //     // -> We proceed with this request.
                    //     DynamicJsonDocument tokenDoc(384);
                    //     tokenDoc["access_token"] = jwtToken;
                    //     jwtToken = "";
                    //     serializeJson(tokenDoc, jwtToken);
                    //     res->print(jwtToken.c_str());
                    // }
                }
                else {
                    res->setStatusText("Authentication failed! Invalid username or password.");
                    res->setStatusCode(401);
                    
                }
            }
            else {
                res->setStatusText("Unauthorized");
                res->setStatusCode(401);
                //res->setHeader("Content-Type", "text/plain");
                
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

            if(doc["exp"].as<unsigned long>() < getTime()){
                res->setStatusCode(401);

            } 
            
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
    string jwtTokenFromCookie = "", jwtTokenExpires = "";
    string request = req->getRequestString().c_str();
    
    bool decodeResult = false;
    bool isLoginPage = strstr(req->getRequestString().substr(0, 6).c_str(), "/login") != nullptr ;
    bool isPostRequest = strstr(req->getMethod().c_str(), "POST") != nullptr ;
    esp32_user_auth_info authResult;

    //check if cookie has auth
    vector<string> chunks = explode(cookieHeader,";",true);
    for(size_t idx = 0; idx < chunks.size(); idx++){
        vector<string> parts = explode(chunks[idx],"=");
        if(parts.size() == 1){ //no name
            #ifdef DEBUG
            Serial.printf("Invalid cookie parsed without name %s\n", parts[0].c_str());
            #endif
        }
        else if(strcmp(parts[0].c_str(),"auth") == 0){
            jwtTokenFromCookie = parts[1];
            //Serial.printf("Found auth header %s in cookie\n", jwtTokenFromCookie.c_str());        
        }
        else if(strcmp(parts[0].c_str(),"expires") == 0){
            jwtTokenExpires = parts[1];
        }
    }
    if(jwtTokenFromCookie.length() > 0){
        //TODO: check if its expired
        bool cookieExpired = strstr(jwtTokenExpires.c_str(),"Thu, 01 Jan 1970 00:00:00 GMT") != nullptr;
        #ifdef DEBUG_SECURITY
        Serial.printf("Checking expiry date of cookie %s from cookie header:\n\t%s\n",jwtTokenExpires.c_str(),cookieHeader.c_str());
        #endif
         if(cookieExpired){
            if(!isLoginPage){
                res->setStatusCode(303);
                res->setHeader("Location", "/login?to=" + request);
                res->setHeader("WWW-Authenticate", "Basic realm=\"Internal\"");
            }
            if(!server.middleware->isPublicPage(req->getRequestString()))
                return;
            else {
                Serial.println("Anonomous access");
                req->setHeader(HEADER_USERNAME, "");
                req->setHeader(HEADER_GROUP, "");
                req->setHeader(HEADER_COOKIE, "");
                jwtTokenFromCookie = ""; //clear cookie data
            }
         }
    }       

    string jwtTokenFromRequest = authHeader.c_str();
    if (jwtTokenFromRequest.length() > 7) { //strip leading "Bearer: "
        //Serial.print("Stripping Bearer from token text\n");
        jwtTokenFromRequest.erase(0,7);
    }

    //if an auth header is passed, it gets priority over cookie
    string jwtToken = jwtTokenFromRequest.length() > 0 ? jwtTokenFromRequest : jwtTokenFromCookie ;
    if(iequals(request.c_str(), "/logout",strlen("/logout"))){
        //redirect to login
        res->setStatusCode(303);
        res->setHeader("Location","/W/logout.html");
        next(); 
        return;
    }    
    
    string jwtDecodedString="";
    jwtDecodedString.reserve(jwtToken.length() * 4);   
    //Serial.printf("Decoding JWT token %s\n", jwtToken.c_str()); 
    if(jwtToken.length() > 0 && server.middleware->jwtTokenizer->decodeJWT(jwtToken,jwtDecodedString))
        decodeResult= true;
    string message = string_format(
        "**Authorization**\\t Decoded: %s\\nJWT Encoded: %s\\nJWT Decoded: %s\\n", decodeResult ? "Yes" : "No",
        jwtToken.c_str(),
        jwtDecodedString.c_str()
    );
    logger.log(message, auth, debug);
    #ifdef DEBUG_SECURITY
    Serial.println(message.c_str());
    #endif
    if (decodeResult) {        
        DynamicJsonDocument doc(jwtDecodedString.length() * 2);
        DeserializationError err = deserializeJson(doc, jwtDecodedString);
        if (err.code() == err.Ok)
        {
            if(isLoginPage && isPostRequest){ //special case for posting login info
                setAuthHeaders(req, doc["user"].as<const char*>(), doc["role"].as<const char*>(),jwtToken.c_str());
                return; 
            }
            //TODO: create config setting to force verify token
            //verify token is valid    
            authResult = esp32_authentication::authenticateUser(doc["user"].as<string>().c_str(),doc["password"].as<string>().c_str());
            //authResult.authenticated = true; //IF YOU WANT TO BYPAS VERIFIACTION OF USER
            #ifdef DEBUG_SECURITY
            Serial.printf(
                "**Authorization** Result %s\n", 
                authResult.authenticated ? "Authenticated" : "Unauthorized"
            ); 
            if(authResult.authenticated)
                Serial.printf(
                    "\tUsername: %s\tRole: %s\n", 
                    authResult.username.c_str(), 
                    authResult.role.c_str()
                );
            #endif
            if(authResult.authenticated){                
            
                setAuthHeaders(req, doc["user"].as<const char*>(), doc["role"].as<const char*>(),jwtToken.c_str());            
                      
            }
        }
        else 
        {
            auto message = string_format(
                    "ERROR [%i] OCCURED DESERIALIZING JWT TOKEN: %s   Details: %s",
                    err.code(),
                    jwtDecodedString.c_str(),
                    err.c_str()
            );
            logger.log( message, auth, error);
            #ifdef DEBUG_SECURITY
            Serial.println(message.c_str());
            #endif  
        }     
        
    }
    // Everything else will be allowed, so we call next()
    if(!authResult.authenticated && denyIfNotPublic(req,res) && !iequals(request.c_str(),"/login",strlen("/login"))){ 
        res->setStatusCode(303);
        res->setHeader("Location", "/login?to=" + request);
        res->setHeader("WWW-Authenticate", "Basic realm=\"Internal\"");
        return;
    }
    if(!authResult.authenticated && denyIfNotAuthorized(req,res)){
        res->setStatusCode(401);
        return;
    }
    //Serial.printf("Request for resource %s authorized.\n", req->getRequestString().c_str());
    next();
}

bool esp32_middleware::denyIfNotPublic(HTTPRequest* req, HTTPResponse* res){
    //bool exclusion = false;
    string request = req->getRequestString();
    if(!server.middleware->isPublicPage(request)){    

        #ifdef DEBUG
        Serial.printf("Request for resource %s rejected. Resource is not public.", request.c_str());
        #endif
        if(ends_with(request,".gz") || ends_with(request, ".js") || ends_with(request, ".css")){
            res->setStatusCode(401);
            return true;
        }
        //if not resource file, redirect user, unless already at login page
        if(!iequals(request.c_str(),"/login",strlen("/login"))){
            res->setStatusCode(303);
            res->setHeader("Location", "/login?to=" + request);
            res->setHeader("WWW-Authenticate", "Basic realm=\"Internal\"");
            return true;
        }
        return true;
    }
    return false;
}

bool esp32_middleware::denyIfNotAuthorized(HTTPRequest* req, HTTPResponse* res){
    bool isInternalPath = req->getRequestString().substr(0, 5) == PATH_INTERNAL_ROOT;
    if(isInternalPath && req->getHeader(HEADER_GROUP) != "ADMIN"){
        #ifdef DEBUG
        Serial.printf("Request for resource %s unauthorized for user %u.", req->getRequestString().c_str(), req->getHeader(HEADER_USERNAME));
        #endif
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

bool esp32_middleware::isPublicPage(string path)
{
    if(path.empty() || path.length() > 32) return false;
    if(_publicPages.size() > 256){
        //public pages corrupted or missing, reload
        return false;//initPublicPages();
    }
    //Serial.printf("Checking if page %s matches one of %d public pages ... \n", path.c_str(), _publicPages.size());
    for(int i = 0; i < _publicPages.size();i++){
        //bool isGz = path.find_last_of(".gz")  == path.length() - 3;
        if(_publicPages[i].empty()) continue;
        //Serial.println(_publicPages[i].c_str());
        int length = _publicPages[i].length();
        if(iequals(_publicPages[i].c_str(), /* isGz ? path.substr(0,path.length() - 3).c_str() : */ path.c_str(),length))
            return true; //page is marked as public
    }
    return false;
}

int esp32_middleware::initPublicPages()
{
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    _publicPages.clear();
    int fileCount = 0;
    if(!drive->exists(PATH_PUBLIC_PAGES)) return 0;
    File publicPagesFile = drive->open(PATH_PUBLIC_PAGES);
    //Serial.println("Adding public pages");
    String line = String("");
    while(publicPagesFile.available()){
       line = publicPagesFile.readStringUntil('\n');
        line.trim();
        if(line.length() == 0) continue;
        if(line[0] != '/') line = "/" + line;
        _publicPages.push_back(line.c_str());
        fileCount++;
        //Serial.printf("Adding public page: %s\n",line.c_str());
    }
    publicPagesFile.close();
    return fileCount;
    //Serial.printf("Added %d public pages\n",fileCount);
}

