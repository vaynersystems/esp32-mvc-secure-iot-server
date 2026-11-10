
#include "esp32_middleware.hpp"
#include "esp32_authentication.hpp"
#include "esp32_key_derivation.h"
#include "../CORE/esp32_wifi.h"
#include <esp_system.h>
#include "../CORE/esp32_server.hpp"
#include "string_helper.h"
#include <esp_task_wdt.h>
#include <BLEEddystoneTLM.h>
extern esp32_server server;
#define STR_LINES PROGMEM "--------------------------------------------------------------------"

/// @brief The page shown in a phone's "sign in to network" window while the device runs its own
/// access point. It never asks for a password; it sends the user to the HTTPS login instead.
static void sendCaptiveLandingPage(HTTPResponse* res){
    string ip = esp32_wifi::captivePortalIp();
    string loginUrl = "https://" + ip + "/login";
    string fingerprint = server.certificateFingerprint();

    res->setStatusCode(200);
    res->setHeader("Content-Type", "text/html; charset=utf-8");
    res->setHeader("Cache-Control", "no-store");
    string h;
    h.reserve(2048);
    h += "<!doctype html><html><head><meta charset=utf-8>"
         "<meta name=viewport content=\"width=device-width,initial-scale=1\"><title>Sign in</title><style>"
         "body{font-family:system-ui,sans-serif;max-width:32rem;margin:1rem auto;padding:0 1rem;color:#222;line-height:1.45}"
         "a.btn{display:inline-block;margin:1rem 0;padding:.7rem 1.4rem;background:#1f6feb;color:#fff;text-decoration:none;border-radius:6px}"
         "code{word-break:break-all;font-size:.85em}small{color:#666}"
         "@media(prefers-color-scheme:dark){body{background:#151515;color:#ddd}small{color:#999}}"
         "</style></head><body><h1>Sign in to this device</h1>"
         "<p>To keep your password private, sign-in only happens over an encrypted (HTTPS) connection. "
         "Open this address in your browser:</p><p><a class=btn href=\"";
    h += loginUrl;
    h += "\">";
    h += loginUrl;
    h += "</a></p>"
         "<p><small>If this window cannot open it: close it and keep the network when asked "
         "(iPhone: <b>Cancel</b> then <b>Use Other Network</b> or <b>Use Without Internet</b>; "
         "Android: <b>Use this network as is</b>), then type the address into your browser.</small></p>";
    if(!fingerprint.empty()){
        h += "<p><small>The browser will warn that the certificate is not trusted, because the device signs its own. "
             "Before continuing, check that the certificate's SHA-256 fingerprint matches the one the device "
             "prints on its serial console:</small><br><code>";
        h += fingerprint;
        h += "</code></p>";
    }
    h += "</body></html>";
    res->print(h.c_str());
}

// Failed logins are throttled per client address. After LOGIN_FREE_ATTEMPTS failures the client
// must wait before trying again, and the wait doubles with each further failure, up to
// LOGIN_MAX_LOCKOUT_MS. A few slots are enough: a device has few clients, and when the table is
// full the oldest entry is reused.
#define LOGIN_THROTTLE_SLOTS 8
#define LOGIN_FREE_ATTEMPTS 5
#define LOGIN_BASE_LOCKOUT_MS 30000UL
#define LOGIN_MAX_LOCKOUT_MS (15UL * 60UL * 1000UL)

struct login_throttle_slot { uint32_t ip; uint8_t failures; unsigned long lastFailure; unsigned long lockedUntil; };
static login_throttle_slot loginThrottle[LOGIN_THROTTLE_SLOTS];

static login_throttle_slot* findLoginSlot(uint32_t ip, bool create){
    login_throttle_slot* oldest = &loginThrottle[0];
    for(auto& slot : loginThrottle){
        if(slot.failures > 0 && slot.ip == ip) return &slot;
        if(slot.lastFailure < oldest->lastFailure || slot.failures == 0) oldest = &slot;
    }
    if(!create) return nullptr;
    *oldest = {ip, 0, 0, 0};
    return oldest;
}

// Device-wide limit as well: the per-address table is small and an attacker can rotate source
// addresses (each new one evicts the oldest entry), so at most LOGIN_GLOBAL_MAX_FAILURES
// failures are accepted per LOGIN_GLOBAL_WINDOW_MS from all clients together. Each failure also
// costs a full PBKDF2 run on the server task, so this keeps a guessing burst from stalling it.
#define LOGIN_GLOBAL_MAX_FAILURES 10
#define LOGIN_GLOBAL_WINDOW_MS 60000UL
static unsigned long globalFailures[LOGIN_GLOBAL_MAX_FAILURES]; // ring of failure times
static uint8_t globalFailureNext = 0;

static unsigned long globalLockoutRemaining(){
    // The oldest of the last N failures: if it is inside the window, N failures happened in it.
    unsigned long oldest = globalFailures[globalFailureNext];
    if(oldest == 0) return 0;
    unsigned long age = millis() - oldest;
    return age < LOGIN_GLOBAL_WINDOW_MS ? LOGIN_GLOBAL_WINDOW_MS - age : 0;
}

/// @return milliseconds the client must still wait, or 0 if it may try to log in
static unsigned long loginLockoutRemaining(uint32_t ip){
    unsigned long global = globalLockoutRemaining();
    login_throttle_slot* slot = findLoginSlot(ip, false);
    unsigned long perClient = 0;
    if(slot != nullptr && slot->lockedUntil != 0){
        long remaining = (long)(slot->lockedUntil - millis());
        perClient = remaining > 0 ? (unsigned long)remaining : 0;
    }
    return global > perClient ? global : perClient;
}

static void recordLoginResult(uint32_t ip, bool success){
    if(success){
        login_throttle_slot* slot = findLoginSlot(ip, false);
        if(slot != nullptr) *slot = {0, 0, 0, 0};
        return;
    }
    globalFailures[globalFailureNext] = millis() == 0 ? 1 : millis();
    globalFailureNext = (globalFailureNext + 1) % LOGIN_GLOBAL_MAX_FAILURES;
    login_throttle_slot* slot = findLoginSlot(ip, true);
    if(slot->failures < 255) slot->failures++;
    slot->lastFailure = millis();
    if(slot->failures >= LOGIN_FREE_ATTEMPTS){
        uint8_t doublings = slot->failures - LOGIN_FREE_ATTEMPTS;
        unsigned long lockout = doublings >= 5 ? LOGIN_MAX_LOCKOUT_MS : LOGIN_BASE_LOCKOUT_MS << doublings;
        if(lockout > LOGIN_MAX_LOCKOUT_MS) lockout = LOGIN_MAX_LOCKOUT_MS;
        slot->lockedUntil = millis() + lockout;
        if(slot->lockedUntil == 0) slot->lockedUntil = 1;
    }
}

void esp32_middleware::middlewareAuthentication(HTTPRequest* req, HTTPResponse* res, std::function<void()> next) {
    bool captive = esp32_wifi::captivePortalActive();
    string hostHeader = req->getHeader("Host");
    bool foreignHost = captive && esp32_wifi::isForeignHost(hostHeader);
    if(captive){
        // Browsers re-check their portal probe every second or two; log each distinct request
        // once a minute at most so the console stays readable.
        static string lastLogged;
        static unsigned long lastLoggedAt = 0;
        string line = string_format("%s %s://%s%s from %s", req->getMethod().c_str(), req->isSecure() ? "https" : "http",
            hostHeader.c_str(), req->getRequestString().c_str(), req->getClientIP().toString().c_str());
        if(line != lastLogged || millis() - lastLoggedAt > 60000){
            Serial.printf("[portal] %s\n", line.c_str());
            lastLogged = line;
            lastLoggedAt = millis();
        }
    }

    // With HTTPS available, plain HTTP never carries a password or a session cookie.
    if(server.sslEnabled() && !req->isSecure()){
        if(foreignHost){
            // An OS connectivity probe (connectivitycheck.gstatic.com, captive.apple.com, ...)
            // that DNS sent to us. Not answering as expected makes the OS show its
            // "sign in to network" window, which lands on the page below.
            res->setStatusCode(302);
            res->setStatusText("Found");
            res->setHeader("Location", esp32_wifi::captivePortalUrl());
            res->setHeader("Cache-Control", "no-store");
        } else if(captive){
            // The sign-in window cannot be trusted with a password (it is plain HTTP, and
            // most OS sign-in windows refuse self-signed certificates), so it only points
            // the user at the HTTPS login.
            sendCaptiveLandingPage(res);
        } else {
            string host = hostHeader.substr(0, hostHeader.find(':'));
            if(host.empty()) host = WiFi.localIP().toString().c_str();
            res->setStatusCode(301);
            res->setStatusText("Moved Permanently");
            res->setHeader("Location", "https://" + host + req->getRequestString());
        }
        return;
    }

    // SSL disabled: there is only the HTTP server, so the captive portal goes straight to login.
    if(foreignHost){
        res->setStatusCode(302);
        res->setStatusText("Found");
        res->setHeader("Location", esp32_wifi::captivePortalUrl() + "login");
        res->setHeader("Cache-Control", "no-store");
        return;
    }

    // Unset both headers to discard any value from the client
    // This prevents authentication bypass by a client that just sets X-USERNAME   
    setAuthHeaders(req,"","","");

    bool jwtTokenValid = false;
    string jwtTokenFromCookie = req->getHeader("Cookie").c_str();
    string jwtToken, jwtTokenFromRequest = req->getBearerAuthToken().c_str();
    string jwtDecodedString, jwtTokenPayload, jwtTokenPayloadVerify;
    string reqUsername;
    string reqPassword;
    string reqSetupCode;
    bool isLoginPage = strstr(req->getRequestString().substr(0, 6).c_str(), "/login") != nullptr ;
    bool isPostRequest = strstr(req->getMethod().c_str(), "POST") != nullptr ;

    if (req->getRequestString().substr(0, 7) == "/logout") {
        jwtTokenFromRequest.clear();
        res->setHeader("Set-Cookie","expires=Thu, 01 Jan 1970 00:00:00 GMT;auth=; path=/");
        req->setHeader(HEADER_AUTH, "");
        res->setHeader(HEADER_GROUP, "");
        // Return after next(): falling through called next() a second time at the end of this
        // function, so the rest of the middleware chain and the handler ran twice for /logout.
        next();
        return;
    }
    else if (isLoginPage && isPostRequest) {
        jwtTokenFromRequest.clear();
        req->setHeader(HEADER_AUTH, "");
        res->setHeader(HEADER_GROUP, "");
        
        //check user/pass.. issue token
        // Leave room for the terminator: a body of 256 bytes or more used to write one past the end.
        char buffer[256];
        size_t reqLength = req->getContentLength();
        if(reqLength > sizeof(buffer) - 1) reqLength = sizeof(buffer) - 1;
        // The body can arrive after the headers (over HTTPS often in its own TLS record), so a
        // single read could return nothing and the login was refused as "no credentials".
        size_t readLength = 0;
        unsigned long readStart = millis();
        while(readLength < reqLength && millis() - readStart < 3000){
            size_t got = req->readChars(buffer + readLength, reqLength - readLength);
            if(got == 0){
                if(req->requestComplete()) break;
                delay(5);
                continue;
            }
            readLength += got;
        }
        if(readLength > reqLength) readLength = reqLength;
        buffer[readLength] = 0;

        // Serial.printf("Read: %s from request\r\n", buffer);
        DynamicJsonDocument doc(sizeof(buffer) * 8);
        DeserializationError err = deserializeJson(doc, buffer);
        if (err.code() == err.Ok) {
            // A missing field must not reach std::string as a null pointer: that crashed the device.
            reqUsername = doc["user_id"] | "";
            reqPassword = doc["password"] | "";
            reqSetupCode = doc["setup_code"] | "";
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
        
        // (This was negated, so a token that failed to decode was treated as valid and the reverse.
        // Note the Authorization header is cleared above, so today the session always comes from
        // the cookie and is checked in middlewareAuthorization.)
        jwtTokenValid = (jwtTokenFromRequest.length() > 0 && server.middleware->jwtTokenizer->decodeJWT(jwtTokenFromRequest, jwtDecodedString));
    }
    
    if (!jwtTokenValid){          
        if(isLoginPage && isPostRequest) {

            uint32_t clientIp = (uint32_t)req->getClientIP();
            unsigned long lockoutMs = loginLockoutRemaining(clientIp);
            // Get login information from request
            if (lockoutMs > 0) {
                res->setStatusCode(429);
                res->setStatusText("Too many failed logins. Try again later.");
                res->setHeader("Retry-After", string_format("%lu", lockoutMs / 1000 + 1));
            }
            else if (reqUsername.length() > 0 && reqPassword.length() > 0 && reqUsername.length() <= 32 && reqPassword.length() <= PASSWORD_MAX_LENGTH) {
                //Serial.println("Checking loging info..");
                esp32_user_auth_info info = esp32_authentication::authenticateUser(reqUsername.c_str(), reqPassword.c_str(), reqSetupCode.c_str());
                recordLoginResult(clientIp, info.authenticated);
                string message = string_format("Authentication for: %s - %s", reqUsername.c_str(), info.authenticated ? "VALID" : "INVALID");
                logger.logInfo(message.c_str(), auth);
                #if defined(DEBUG_SECURITY) && DEBUG_SECURITY > 0
                Serial.println(message.c_str());
                #endif
                
                
                // if authentication was successful issue JWT token
                if (info.authenticated) {
                    DynamicJsonDocument doc(512);
                    doc["user"] = info.username;
                    doc["role"] = info.role.c_str();
                    // Never put password material in the token: a JWT is signed, not encrypted.
                    // The session id only lets the server revoke the token (rotated on password change).
                    doc["sid"] = info.sessionId;
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
                else if (info.setupRequired) {
                    // The login page shows the setup code field when it sees this text.
                    res->setStatusText("Setup code required. It is printed on the device's serial console.");
                    res->setStatusCode(401);
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
            
            // Pass the verified token on; middlewareAuthorization checks its session and sets the
            // user and role. (This passed an empty string, which would have discarded the token.)
            setAuthHeaders(req, "","",jwtTokenFromRequest.c_str());
        }
        else {
            // Failed to deserialize token
#if defined(DEBUG_SECURITY) && DEBUG_SECURITY > 0
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
            #if defined(DEBUG_SECURITY) && DEBUG_SECURITY > 0
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
                #if defined(DEBUG_SECURITY) && DEBUG_SECURITY > 0
                Serial.println("Anonomous access");
                #endif
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
        // Revoke the session on the device, not just the browser's cookie: a copied token
        // must stop working once its owner logs out.
        string logoutPayload;
        if(jwtToken.length() > 0 && server.middleware->jwtTokenizer->decodeJWT(jwtToken, logoutPayload)){
            DynamicJsonDocument logoutDoc(logoutPayload.length() * 2 + 64);
            if(!deserializeJson(logoutDoc, logoutPayload))
                esp32_authentication::revokeSession(logoutDoc["user"] | "", logoutDoc["sid"] | "");
        }
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
    #ifdef DEBUG_SECURITY
    logger.log(message, auth, debug);    
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
            //verify token has not expired and its session is still valid for this user
            unsigned long now = getTime();
            unsigned long exp = doc["exp"] | 0UL;
            bool expired = exp == 0 || (now > 0 && exp < now);
            if(!expired)
                authResult = esp32_authentication::validateSession(doc["user"] | "", doc["sid"] | "");
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
                // role comes from the user database so role changes apply immediately
                setAuthHeaders(req, authResult.username.c_str(), authResult.role.c_str(),jwtToken.c_str());            
                      
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
    // Internal folders are admin-only for everyone, including logged-in non-admin users
    // (this used to run only for requests without a valid session).
    if(denyIfNotAuthorized(req,res)){
        res->setStatusCode(authResult.authenticated ? 403 : 401);
        return;
    }
    //Serial.printf("Request for resource %s authorized.\n", req->getRequestString().c_str());
    next();
}

bool esp32_middleware::denyIfNotPublic(HTTPRequest* req, HTTPResponse* res){
    //bool exclusion = false;
    string request = req->getRequestString();
    if(!server.middleware->isPublicPage(request)){    

        #if defined(DEBUG_SECURITY) && DEBUG_SECURITY > 0
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
    // Second line of defence for the internal folders (the router checks the resolved path).
    // This compared substr(0, 5) with the 11-character "/spiffs/INT", which never matched.
    std::string requested = req->getRequestString();
    auto under = [&](const std::string& root){
        return requested.size() >= root.size() && strncasecmp(requested.c_str(), root.c_str(), root.size()) == 0 &&
               (requested.size() == root.size() || requested[root.size()] == '/' || requested[root.size()] == '?');
    };
    bool isInternalPath = under(PATH_INTERNAL_ROOT) || under("/INT") || under(PATH_TEMP_ROOT) || under("/TMP") ||
                          under(PATH_LOGGING_ROOT);
    if(isInternalPath && req->getHeader(HEADER_GROUP) != "ADMIN"){
        #if defined(DEBUG_SECURITY) && DEBUG_SECURITY > 0
        Serial.printf("Request for resource %s unauthorized for user %u.", req->getRequestString().c_str(), req->getHeader(HEADER_USERNAME));
        #endif
        res->setStatusCode(401);
        res->setHeader("Location", req->getHeader("Origin"));
        res->setHeader("WWW-Authenticate", "Basic realm=\"Internal\"");
        return true;
    }
    return false;
}

void esp32_middleware::middlewareSetTokenizer(const unsigned char* pkData, size_t pkLength) {
    // Derive a full-strength HS256 key from the whole private key. The raw DER key must not be
    // used as a C string: it contains a 0x00 byte within the first few bytes, which truncated
    // the signing key to ~6 predictable bytes and made tokens forgeable.
    uint8_t key[DERIVED_KEY_SIZE];
    if(!derive_device_key(pkData, pkLength, KEY_LABEL_JWT, key)){
        // fail closed: random key, tokens will not survive a reboot but cannot be forged
        esp_fill_random(key, sizeof(key));
        logger.logError("Failed to derive JWT key from certificate; using ephemeral key");
    }
    jwtTokenizer = new ArduinoJWT(string((const char*)key, sizeof(key)));
    memset(key, 0, sizeof(key));
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

