#include "esp32_device_service.hpp"
#include "System/MODULES/DEVICES/esp32_devices.hpp"
extern const int SERVER_STACK_SIZE;
extern esp32_devices deviceManager;
DerivedService<esp32_device_service> esp32_device_service::reg("devices");

string esp32_device_service::Execute()
{
    string ret = "";

    if (strcmp(route.params.c_str(),"ping") == 0){
        return "pong";
    }   
    if (strcmp(route.params.c_str(),"snapshot") == 0){
        auto doc =  deviceManager.getLastSnapshot();
        #if DEBUG_DEVICE > 1
        serializeJson(doc, Serial);
        #endif
        auto err = serializeJson(doc, ret);
       
        return ret;
    } 
    return string_format("Request to service `devices` with %s cannot be processed", route.params.length() == 0 ? " missing parameters" : route.params.c_str());
    
}
