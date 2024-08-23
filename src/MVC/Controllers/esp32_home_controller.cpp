#include "esp32_home_controller.hpp"


DerivedController<esp32_home_controller> esp32_home_controller::reg("esp32_home");

void esp32_home_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    
    title = "ESP32 Web Server Home Page";

    
    controllerTemplate.SetTemplateVariable("$_Controllers", GetControllersJSON().c_str());
    
    esp32_base_controller::Index(req,res);    
}

