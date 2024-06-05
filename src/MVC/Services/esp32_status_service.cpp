#include "esp32_status_service.hpp"
extern const int SERVER_STACK_SIZE;

DerivedService<esp32_status_service> esp32_status_service::reg("status");

string esp32_status_service::Execute()
{
    //if connection test, return pong
    if (strcmp(route.params.c_str(),"ping") == 0)
        return "pong";
        
    auto stackFreeBytes = uxTaskGetStackHighWaterMark(NULL); 

    string ret = string_format("{\"UPTIME\": \"%lu\", \"HEAP_FREE\": \"%lu\", \"HEAP_USED\": \"%lu\", \"HEAP_TOTAL\": \"%lu\", \"HEAP_PERCENT_USED\": \"%d\", \"STACK_FREE\": \"%d\", \"STACK_USED\": \"%d\", \"STACK_TOTAL\": \"%d\", \"STACK_PERCENT_USED\": \"%d\"}",
        millis() / 1000,
        esp_get_free_heap_size(),
        ESP.getHeapSize() - esp_get_free_heap_size(),
        ESP.getHeapSize(),
        (int)round(((float)(ESP.getHeapSize() - esp_get_free_heap_size())/(float)ESP.getHeapSize())*100),

        stackFreeBytes,
        SERVER_STACK_SIZE - stackFreeBytes,
        SERVER_STACK_SIZE,
        (int)round(((float)(SERVER_STACK_SIZE - stackFreeBytes)/(float)SERVER_STACK_SIZE)*100)
    );
    return ret;
}
