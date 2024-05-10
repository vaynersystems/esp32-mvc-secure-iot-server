#include "esp32_login_controller.hpp"

DerivedController<esp32_login_controller> esp32_login_controller::reg("login");

void esp32_login_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    esp32_base_controller::Index(req,res);    
}

inline void esp32_login_controller::Post(HTTPRequest *req, HTTPResponse *res)
{
    esp32_base_controller::Post(req,res);
}
