#include "string_extensions.h"
#include <nvs.h>
#include <esp_ota_ops.h>
#include "esp32_status_service.hpp"
extern const int SERVER_STACK_SIZE;
DerivedService<esp32_status_service> esp32_status_service::reg("status");

string esp32_status_service::Execute()
{
    //if connection test, return pong
    if (strcmp(route.params.c_str(),"ping") == 0)
        return "pong";
    
    StaticJsonDocument<2048> doc;

    //get info
    nvs_stats_t stats;
    nvs_get_stats(NULL, &stats); 
    auto stackFreeBytes = uxTaskGetStackHighWaterMark(NULL); 
    
    doc["UPTIME"] = millis() / 1000;
    doc["HEAP_FREE"] = esp_get_free_heap_size();//freeBytesHEAPSPretty.c_str();
    doc["HEAP_USED"] = ESP.getHeapSize() - esp_get_free_heap_size();// usedBytesHEAPPretty.c_str();
    doc["HEAP_TOTAL"] = ESP.getHeapSize();// totalBytesHEAPPretty.c_str();
    doc["HEAP_PERCENT_USED"] = round(((float)(ESP.getHeapSize() - esp_get_free_heap_size())/(float)ESP.getHeapSize())*100);//  to_string_with_precision(round(((float)(ESP.getHeapSize() - esp_get_free_heap_size())/(float)ESP.getHeapSize())*100),1).c_str();
    //doc["HEAP_AVAILABLE"] = ESP.getHeapSize() > 0 ? "block" : "none";    

    doc["NVS_FREE"] = stats.free_entries;// (to_string_with_precision(stats.free_entries,0) + " bytes").c_str();
    doc["NVS_USED"] = stats.used_entries;// (to_string_with_precision(stats.used_entries,0)+ " bytes").c_str();
    doc["NVS_TOTAL"] = stats.total_entries;// (to_string_with_precision(stats.total_entries,0)+ " bytes").c_str();
    doc["NVS_PERCENT_USED"] = round(((float)stats.used_entries/(float)stats.total_entries)*100);// (to_string_with_precision(round(((float)stats.used_entries/(float)stats.total_entries)*100),1).c_str());
	//doc["NVS_AVAILABLE"] =  ESP.getSketchSize() + ESP.getFreeSketchSpace() > 0 ? "block" : "none";

    doc["STACK_FREE"] = stackFreeBytes;// freeBytesSTACKPretty.c_str();
    doc["STACK_USED"] = SERVER_STACK_SIZE - stackFreeBytes;// usedBytesSTACKPretty.c_str();
    doc["STACK_TOTAL"] = SERVER_STACK_SIZE;// totalBytesSTACKPretty.c_str();
    doc["STACK_PERCENT_USED"] = round(((float)(SERVER_STACK_SIZE - stackFreeBytes)/(float)SERVER_STACK_SIZE)*100);// (to_string_with_precision(round(((float)(SERVER_STACK_SIZE - stackFreeBytes)/(float)SERVER_STACK_SIZE)*100),1).c_str());    


    string ret = "";
    serializeJson(doc,ret);
    // serializeJson(doc,Serial);
    return ret;
}
