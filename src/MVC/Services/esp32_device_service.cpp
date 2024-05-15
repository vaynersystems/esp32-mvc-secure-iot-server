#include "esp32_device_service.hpp"
#include "System/MODULES/DEVICES/esp32_devices.hpp"
extern const int SERVER_STACK_SIZE;
extern esp32_devices deviceManager;
DerivedService<esp32_device_service> esp32_device_service::reg("device");

string esp32_device_service::Execute()
{
    if (strcmp(route.params.c_str(),"config") == 0){
        StaticJsonDocument<2048> devicesConfig;
        esp32_config::getConfigSection("devices", &devicesConfig);
        
        string ret = "";        
        serializeJson(devicesConfig, ret);
        return ret;
    }   
    if (strcmp(route.params.c_str(),"snapshot") == 0){
        auto doc =  deviceManager.getLastSnapshot();
        string ret = "";
        serializeJson(*doc, ret);
        return ret;
    }       
    
}
