#include "esp32_status_service.hpp"
extern const int SERVER_STACK_SIZE;

DerivedService<esp32_status_service> esp32_status_service::reg("status");

string esp32_status_service::Execute()
{
    //if connection test, return pong
    if (strcmp(route.params.c_str(),"ping") == 0)
        return "pong";
        
    auto stackFreeBytes = uxTaskGetStackHighWaterMark(NULL); 

    multi_heap_info_t heapInfo;
    heap_caps_get_info(&heapInfo, MALLOC_CAP_INTERNAL);    

    
    

    string ret = string_format("{\"UPTIME\": \"%lu\", \"HEAP_FREE\": \"%lu\", \"HEAP_USED\": \"%lu\", \"HEAP_TOTAL\": \"%lu\", \"HEAP_PERCENT_USED\": \"%d\", \"STACK_FREE\": \"%d\", \"STACK_USED\": \"%d\", \"STACK_TOTAL\": \"%d\", \"STACK_PERCENT_USED\": \"%d\", \"PSRAM_FREE\": \"%d\", \"PSRAM_USED\": \"%d\", \"PSRAM_TOTAL\": \"%d\", \"PSRAM_PERCENT_USED\": \"%d\"}",
        
        millis() / 1000,
        heapInfo.total_free_bytes,
        heapInfo.total_allocated_bytes,
        heapInfo.total_allocated_bytes + heapInfo.total_free_bytes,
        (int)round(((float)(heapInfo.total_allocated_bytes)/(float)(heapInfo.total_allocated_bytes + heapInfo.total_free_bytes))*100),

        stackFreeBytes,
        SERVER_STACK_SIZE - stackFreeBytes,
        SERVER_STACK_SIZE,
        (int)round(((float)(SERVER_STACK_SIZE - stackFreeBytes)/(float)SERVER_STACK_SIZE)*100),
        
        ESP.getFreePsram(),
        ESP.getPsramSize() - ESP.getFreePsram(),
        ESP.getPsramSize(),
        (int)round(((float)(ESP.getPsramSize() - ESP.getFreePsram())/(float)ESP.getPsramSize())*100)        

    );
    return ret;
}
