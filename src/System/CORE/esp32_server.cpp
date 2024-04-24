#include "esp32_server.h"
#include <HTTPMiddlewareFunction.hpp>

esp32_server::esp32_server(SSLCert* cert) : secureServer(new HTTPSServer(cert)), unsecureServer(new HTTPServer())
{
    _cert = cert;
    
    // Add the middleware. These functions will be called globally for every request
    // Note: The functions are called in the order they are added to the server.
    // This means, we need to add the authentication middleware first, because the
    // authorization middleware needs the headers that will be set by the authentication
    // middleware (First we check the identity, then we see what the user is allowed to do)
    secureServer->addMiddleware(middleware->middlewareAuthentication);
    secureServer->addMiddleware(middleware->middlewareAuthorization);
    unsecureServer->addMiddleware(middleware->middlewareAuthentication);
    unsecureServer->addMiddleware(middleware->middlewareAuthorization);
}

bool esp32_server::start() {
    

    _router = new esp32_router();
    middleware = new esp32_middleware();
    middleware->middlewareSetTokenizer((char*)_cert->getPKData());
    _router->RegisterHandlers(SPIFFS, SITE_ROOT, 3);
    _router->RegisterHandler( "/", HTTPMETHOD_GET, &esp32_router::handleRoot);
    _router->RegisterHandler( "/internal", HTTPMETHOD_GET, &esp32_router::handleInternalPage);
    _router->RegisterHandler( "/internal/admin", HTTPMETHOD_GET, &esp32_router::handleAdminPage);
    _router->RegisterHandler( "/public", HTTPMETHOD_GET, &esp32_router::handlePublicPage);
    //RegisterHandler("/index.htm", HTTPMETHOD_GET, &handleEditPage);
    //RegisterHandler("/ace.js", HTTPMETHOD_GET, &handleJSPage);

    _router->RegisterHandler( "/list", HTTPMETHOD_GET, &esp32_router::handleFileList);
    _router->RegisterHandler( "/logout", HTTPMETHOD_GET, &esp32_router::dummyPageHandler);
    // _router->RegisterHandler( "/login", HTTPMETHOD_GET, &esp32_router::dummyPageHandler);
    // _router->RegisterHandler( "/login", HTTPMETHOD_POST, &esp32_router::dummyPageHandler);
    _router->RegisterHandler( "/edit", HTTPMETHOD_PUT, &esp32_router::handleFileUpload);
    _router->RegisterHandler( "/edit", HTTPMETHOD_POST, &esp32_router::handleFileUpload);
    _router->RegisterHandler( "/edit", HTTPMETHOD_DELETE, &esp32_router::handleFileUpload);
    ResourceNode* node404 = new ResourceNode("", "GET", &esp32_router::handle404);
    ResourceNode* nodeRoot = new ResourceNode("", "GET", &esp32_router::handleRoot);
    ResourceNode* nodeSpecial = new ResourceNode("/special/*", "GET", &esp32_router::handleFileList);
    ResourceNode* corsNode = new ResourceNode("/*", "OPTIONS", &esp32_router::handleCORS);
    _router->RegisterHandler( corsNode);
    _router->RegisterHandler( node404);
    _router->RegisterHandler( nodeSpecial);
    //_router->RegisterHandler("/esp32_*", HTTPMETHOD_GET, &esp32_router::handleRoot);

    // Add the 404 not found node to the server.
    // The path is ignored for the default node.
    secureServer->setDefaultNode(nodeRoot);
    unsecureServer->setDefaultNode(nodeRoot);
    
    Serial.println("Starting server...");
    secureServer->start();
    unsecureServer->start();
    if (secureServer->isRunning() && unsecureServer->isRunning()) {
        Serial.println("Server ready.");
        /*esp32_server::timer = timerBegin(0, getCpuFrequencyMhz() / 1000000, true);
        timerAttachInterrupt(timer, server_loop, true);
        timerAlarmWrite(timer,1,true);
        timerAlarmEnable(timer);
        timerStart(timer);*/
    }

    
    
    return secureServer->isRunning() && unsecureServer->isRunning();
}

bool esp32_server::stop()
{
    if (isRunning()){
        secureServer->stop();
        unsecureServer->stop();
    }
    return true;
}

bool esp32_server::isRunning() {
    return secureServer->isRunning() && unsecureServer->isRunning();
}

void esp32_server::step()
{
    secureServer->loop();
    unsecureServer->loop();
}


// void esp32_server::DisplayLoginPage(HTTPResponse* res) {

//     String path = SITE_ROOT;
//     path += "/login.html";
//     File f = SPIFFS.open(path);
//     res->setStatusCode(200);
//     res->setStatusText("OK");
//     res->setHeader("Content-Type", "text/html");
//     res->println(f.readString());
//     //res->println(htmlLogin);
// }
void esp32_server::DisplayErrorPage(HTTPResponse* res, String errorMessage) {
    // Display error page
    res->setStatusCode(401);
    res->setStatusText("Unauthorized");
    res->setHeader("Content-Type", "text/html");
    res->setHeader("X-ERROR", errorMessage.c_str());
    //res->println(htmlLogin);
    String path = SITE_ROOT;
    path += "/index.html";
    File f = SPIFFS.open(path);

    res->println(f.readString());
    /*res->println("<input type='text' name='user' id='user'></input></br>");
    res->println("<input type='password' name='password' id='password'></input></br>");
    res->println("<input type='button' onclick='signin()'/>");*/
    // This should trigger the browser user/password dialog, and it will tell
    // the client how it can authenticate
    //res->setHeader("WWW-Authenticate", "Basic realm=\"ESP32 privileged area\"");

    // Small error text on the response document. In a real-world scenario, you
    // shouldn't display the login information on this page, of course ;-)
    //res->println("401. Unauthorized [Invalid Credentials] (try admin/secret or user/test)");
    Serial.println(errorMessage);
    // NO CALL TO next() here, as the authentication failed.
    // -> The code above did handle the request already.
}

bool esp32_server::RegisterNewCert(SSLCert* cert)
{
    if (secureServer->isRunning())
    {
        ESP_LOGD("Stopping server to register new cert", ESP_LOG_INFO);
        secureServer->stop();
    }

    secureServer =new HTTPSServer(cert);
    _cert = cert;
}
