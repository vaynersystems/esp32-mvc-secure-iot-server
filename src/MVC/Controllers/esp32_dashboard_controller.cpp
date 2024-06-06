#include "esp32_dashboard_controller.hpp"
#include "System/ROUTER/esp32_template.h"
#include "string_helper.h"
#include <nvs.h>
#include <esp_ota_ops.h>
#include <System/CORE/esp32_config.h>

extern const int SERVER_STACK_SIZE;
DerivedController<esp32_dashboard_controller> esp32_dashboard_controller::reg("esp32_dashboard");

void esp32_dashboard_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    
    //this page will render charts for devices.
    // set device config to template variable
    StaticJsonDocument<2048> doc;
    esp32_config::getConfigSection("devices",&doc);
    string deviceString = "";
    serializeJson(doc, deviceString);
    controllerTemplate.SetTemplateVariable(F("$_DEVICES"),deviceString.c_str() );
    esp32_base_controller::Index(req,res);      
}

