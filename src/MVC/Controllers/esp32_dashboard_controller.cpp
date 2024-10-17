#include "esp32_dashboard_controller.hpp"
#include "System/ROUTER/esp32_template.h"
#include "string_helper.h"
#include <nvs.h>
#include "System/CORE/esp32_config.hpp"
#include "System/MODULES/DEVICES/esp32_devices.hpp"

extern const int SERVER_STACK_SIZE;
extern esp32_devices deviceManager;
DerivedController<esp32_dashboard_controller> esp32_dashboard_controller::reg("esp32_dashboard");

void esp32_dashboard_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    
    StaticJsonDocument<2048> configFile;
    auto drive = filesystem.getDisk(0);
    File f = drive->open(PATH_DEVICE_CONFIG,"r");
    auto error = deserializeJson(configFile, f);
    if(error.code() != ESP_OK){
        Serial.printf("Failed to get devices. Received error deserializing configuration file. \n\t %d %s\n", error.code(), error.c_str());
    }
    f.close();       
    
    if(configFile["devices"].isNull()) return;
        
    auto devicesConfig = configFile["devices"].as<JsonArray>();    
    
    string deviceString = "";
    serializeJson(devicesConfig, deviceString);
    controllerTemplate.SetTemplateVariable(F("$_DEVICES"),deviceString.c_str() );
    esp32_base_controller::Index(req,res);      
}

