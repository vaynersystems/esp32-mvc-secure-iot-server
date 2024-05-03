#include "esp32_login_controller.hpp"

DerivedController<esp32_login_controller> esp32_login_controller::reg("login");

void esp32_login_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    Base_Controller::Index(req,res);    
}












