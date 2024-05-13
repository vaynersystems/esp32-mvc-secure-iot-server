#include "esp32_status_service.hpp"
extern const int SERVER_STACK_SIZE;

DerivedService<esp32_status_service> esp32_status_service::reg("status");

string esp32_status_service::Execute()
{
    //if connection test, return pong
    if (strcmp(route.params.c_str(),"ping") == 0)
        return "pong";
    
    StaticJsonDocument<2048> doc;
    
    auto stackFreeBytes = uxTaskGetStackHighWaterMark(NULL); 
    
    doc["UPTIME"] = millis() / 1000;
    doc["HEAP_FREE"] = esp_get_free_heap_size();
    doc["HEAP_USED"] = ESP.getHeapSize() - esp_get_free_heap_size();
    doc["HEAP_TOTAL"] = ESP.getHeapSize();
    doc["HEAP_PERCENT_USED"] = round(((float)(ESP.getHeapSize() - esp_get_free_heap_size())/(float)ESP.getHeapSize())*100);
 
	
    doc["STACK_FREE"] = stackFreeBytes;
    doc["STACK_USED"] = SERVER_STACK_SIZE - stackFreeBytes;
    doc["STACK_TOTAL"] = SERVER_STACK_SIZE;
    doc["STACK_PERCENT_USED"] = round(((float)(SERVER_STACK_SIZE - stackFreeBytes)/(float)SERVER_STACK_SIZE)*100);

    string ret = "";
    serializeJson(doc,ret);
    return ret;
}
