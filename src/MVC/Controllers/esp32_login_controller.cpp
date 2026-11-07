#include "esp32_login_controller.hpp"
#include "System/AUTH/esp32_authentication.hpp"

DerivedController<esp32_login_controller> esp32_login_controller::reg("login");

void esp32_login_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    // Lets the page show the setup code field up front while no users exist, instead of
    // only after a first login attempt is refused.
    controllerTemplate.SetTemplateVariable(F("$_SETUP_REQUIRED"), esp32_authentication::setupRequired() ? "true" : "false");
    esp32_base_controller::Index(req,res);    
}

inline void esp32_login_controller::Post(HTTPRequest *req, HTTPResponse *res)
{
    esp32_base_controller::Post(req,res);
}
