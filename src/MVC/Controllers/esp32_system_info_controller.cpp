#include "esp32_system_info_controller.hpp"
#include "System/ROUTER/esp32_template.h"
#include "string_helper.h"
#include <nvs.h>
#include <esp_ota_ops.h>
#include <SD.h>

extern const int SERVER_STACK_SIZE;
DerivedController<esp32_system_info_controller> esp32_system_info_controller::reg("esp32_system_info");

void esp32_system_info_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    
    //auto spiffs_mem = esp32_fileio::getMemoryInfo(SPIFFS);
    auto spiffs_info = filesystem.getDisk("spiffs")->info();
    auto disk = filesystem.getDisk("sd");
    esp32_drive_info sd_info;
    if(disk != NULL)
        sd_info = disk->info();
    nvs_stats_t stats;
    nvs_get_stats(NULL, &stats);    

    string bootSizePretty("0"), part1SizePretty("0"); 
    
    int partitions = esp_ota_get_app_partition_count();
    const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
    esp32_fileio::PrettyFormat(boot_partition->size, &bootSizePretty);    
    if(partitions > 1){
        const esp_partition_t *app_partition = esp_ota_get_next_update_partition(boot_partition);
        esp32_fileio::PrettyFormat(app_partition->size, &part1SizePretty);
    }   
    
   
    controllerTemplate.SetTemplateVariable(F("$_PARTITION_BOOT_SPACE"),  bootSizePretty.c_str());    
    controllerTemplate.SetTemplateVariable(F("$_PARTITION_1_SPACE"), part1SizePretty.c_str());

    string flashSizePretty(""), sketchSizePretty(""), flashMode; 
    esp32_fileio::PrettyFormat((size_t)ESP.getFlashChipSize(), &flashSizePretty);
    esp32_fileio::PrettyFormat((size_t)ESP.getSketchSize(), &sketchSizePretty);
    prettyFlashModeString(flashMode);

    controllerTemplate.SetTemplateVariable(F("$_CHIP_CORES"),      to_string(ESP.getChipCores()).c_str());
    controllerTemplate.SetTemplateVariable(F("$_CHIP_MODEL"),      ESP.getChipModel());
    controllerTemplate.SetTemplateVariable(F("$_CHIP_REVISION"),   to_string(ESP.getChipRevision()).c_str());
    controllerTemplate.SetTemplateVariable(F("$_CPU_FREQUENCY"),   to_string(ESP.getCpuFreqMHz()).c_str());
    controllerTemplate.SetTemplateVariable(F("$_FLASH_MODE"),      flashMode.c_str());
    controllerTemplate.SetTemplateVariable(F("$_FLASH_SIZE"),      flashSizePretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$_FLASH_SPEED"),     to_string(ESP.getFlashChipSpeed()).c_str());
    controllerTemplate.SetTemplateVariable(F("$_SDK_VERSION"),     ESP.getSdkVersion());
    controllerTemplate.SetTemplateVariable(F("$_SKETCH_SIZE"),     sketchSizePretty.c_str()) ;
    controllerTemplate.SetTemplateVariable(F("$_FIRMWARE_VERSION"),FIRMWARE_VERSION) ;
    
    



    string freeBytesSPIFFSPretty(""), totalBytesSPIFFSPretty(""), usedBytesSPIFFSPretty("");
    esp32_fileio::PrettyFormat(spiffs_info.size() - spiffs_info.used(), &freeBytesSPIFFSPretty);
    esp32_fileio::PrettyFormat(spiffs_info.used(), &usedBytesSPIFFSPretty);
    esp32_fileio::PrettyFormat(spiffs_info.size(), &totalBytesSPIFFSPretty);

    controllerTemplate.SetTemplateVariable(F("$SPIFFS_MEMORY_FREE"), freeBytesSPIFFSPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$SPIFFS_MEMORY_USED"), usedBytesSPIFFSPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$SPIFFS_MEMORY_TOTAL"), totalBytesSPIFFSPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$SPIFFS_MEMORY_PERCENT_USED"), to_string_with_precision(round(((float)spiffs_info.used()/(float)spiffs_info.size())*100),1).c_str());
    controllerTemplate.SetTemplateVariable(F("$SPIFFS_MEMORY_AVAILABLE"), spiffs_info.size() > 0 ? "block" : "none");

    /* SD */
    string freeBytesSDPretty(""), totalBytesSDPretty(""), usedBytesSDPretty("");
    esp32_fileio::PrettyFormat(sd_info.size() - sd_info.used(), &freeBytesSDPretty);
    esp32_fileio::PrettyFormat(sd_info.used(), &usedBytesSDPretty);
    esp32_fileio::PrettyFormat(sd_info.size(), &totalBytesSDPretty);

    controllerTemplate.SetTemplateVariable(F("$SD_MEMORY_FREE"), freeBytesSDPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$SD_MEMORY_USED"), usedBytesSDPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$SD_MEMORY_TOTAL"), totalBytesSDPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$SD_MEMORY_PERCENT_USED"), to_string_with_precision(round(((float)sd_info.used()/(float)sd_info.size())*100),1).c_str());
    controllerTemplate.SetTemplateVariable(F("$SD_MEMORY_AVAILABLE"), sd_info.size() > 0 ? "block" : "none");
    
    /* HEAP */
    string freeBytesHEAPSPretty(""), usedBytesHEAPPretty("") , totalBytesHEAPPretty("");
	esp32_fileio::PrettyFormat((size_t)esp_get_free_heap_size(), &freeBytesHEAPSPretty);
	esp32_fileio::PrettyFormat((size_t)ESP.getHeapSize(), &totalBytesHEAPPretty);
    esp32_fileio::PrettyFormat((size_t)ESP.getHeapSize() - esp_get_free_heap_size(), &usedBytesHEAPPretty);

    controllerTemplate.SetTemplateVariable(F("$HEAP_MEMORY_FREE"), freeBytesHEAPSPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$HEAP_MEMORY_USED"), usedBytesHEAPPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$HEAP_MEMORY_TOTAL"), totalBytesHEAPPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$HEAP_MEMORY_PERCENT_USED"), to_string_with_precision(round(((float)(ESP.getHeapSize() - esp_get_free_heap_size())/(float)ESP.getHeapSize())*100),1).c_str());
    controllerTemplate.SetTemplateVariable(F("$HEAP_MEMORY_AVAILABLE"), ESP.getHeapSize() > 0 ? "block" : "none");

    /* PSRAM */
    string freeBytesPSRAMPretty(""), usedBytesPSRAMPretty("") , totalBytesPSRAMPretty("");
	esp32_fileio::PrettyFormat(ESP.getFreePsram(), &freeBytesPSRAMPretty);
	esp32_fileio::PrettyFormat(ESP.getPsramSize(), &totalBytesPSRAMPretty);
    esp32_fileio::PrettyFormat(ESP.getPsramSize() - ESP.getFreePsram(), &usedBytesPSRAMPretty);

    controllerTemplate.SetTemplateVariable(F("$PSRAM_MEMORY_FREE"), freeBytesPSRAMPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$PSRAM_MEMORY_USED"), usedBytesPSRAMPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$PSRAM_MEMORY_TOTAL"), totalBytesPSRAMPretty.c_str());    
    controllerTemplate.SetTemplateVariable(F("$PSRAM_MEMORY_PERCENT_USED"), to_string_with_precision(round(((float)(ESP.getPsramSize() - ESP.getFreePsram())/(float)ESP.getPsramSize())*100),1).c_str());
    controllerTemplate.SetTemplateVariable(F("$PSRAM_MEMORY_AVAILABLE"), ESP.getPsramSize() > 0 ? "block" : "none");

    /* SKETCH */
    string freeBytesSKETCHPretty(""), usedBytesSKETCHPretty("") , totalBytesSKETCHPretty("");
	esp32_fileio::PrettyFormat(ESP.getFreeSketchSpace(), &freeBytesSKETCHPretty);
	esp32_fileio::PrettyFormat(ESP.getSketchSize() + ESP.getFreeSketchSpace(), &totalBytesSKETCHPretty);
    esp32_fileio::PrettyFormat(ESP.getSketchSize(), &usedBytesSKETCHPretty);

    controllerTemplate.SetTemplateVariable(F("$SKETCH_MEMORY_FREE"), freeBytesSKETCHPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$SKETCH_MEMORY_USED"), usedBytesSKETCHPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$SKETCH_MEMORY_TOTAL"), totalBytesSKETCHPretty.c_str());    
    controllerTemplate.SetTemplateVariable(F("$SKETCH_MEMORY_PERCENT_USED"), to_string_with_precision(round(((float)ESP.getSketchSize()/(float)(ESP.getSketchSize() + ESP.getFreeSketchSpace()))*100),1).c_str());
    controllerTemplate.SetTemplateVariable(F("$SKETCH_MEMORY_AVAILABLE"), ESP.getSketchSize() + ESP.getFreeSketchSpace() > 0 ? "block" : "none");
    

    controllerTemplate.SetTemplateVariable(F("$NVS_FREE_ENTRIES"), (to_string_with_precision(stats.free_entries,0)).c_str());
    controllerTemplate.SetTemplateVariable(F("$NVS_USED_ENTRIES"), (to_string_with_precision(stats.used_entries,0)).c_str());
    controllerTemplate.SetTemplateVariable(F("$NVS_TOTAL_ENTRIES"), (to_string_with_precision(stats.total_entries,0)).c_str());
    controllerTemplate.SetTemplateVariable(F("$NVS_PERCENT_USED"), (to_string_with_precision(round(((float)stats.used_entries/(float)stats.total_entries)*100),1).c_str()));
	controllerTemplate.SetTemplateVariable(F("$NVS_MEMORY_AVAILABLE"), ESP.getSketchSize() + ESP.getFreeSketchSpace() > 0 ? "block" : "none");
    //Print debug message
    //controllerTemplate.PrintDebugMessage(req,res);
    auto stackFreeBytes = uxTaskGetStackHighWaterMark(NULL);     
    string freeBytesSTACKPretty(""), usedBytesSTACKPretty("") , totalBytesSTACKPretty("");
	esp32_fileio::PrettyFormat(stackFreeBytes, &freeBytesSTACKPretty);
	esp32_fileio::PrettyFormat(SERVER_STACK_SIZE - stackFreeBytes, &usedBytesSTACKPretty);
    esp32_fileio::PrettyFormat(SERVER_STACK_SIZE, &totalBytesSTACKPretty);
    controllerTemplate.SetTemplateVariable(F("$STACK_FREE"), freeBytesSTACKPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$STACK_USED"), usedBytesSTACKPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$STACK_TOTAL"), totalBytesSTACKPretty.c_str());
    controllerTemplate.SetTemplateVariable(F("$STACK_PERCENT_USED"), (to_string_with_precision(round(((float)(SERVER_STACK_SIZE - stackFreeBytes)/(float)SERVER_STACK_SIZE)*100),1).c_str()));    

    // char * varName = new char[20];
    // for(int i = 1; i < 100; i++){
    //     memset(varName,0,20);
    //     sprintf(varName,"$_TEST_%d",i);
    //     controllerTemplate.SetTemplateVariable(varName,"SOME LONG TESTING STRING TO SEE IF THE SIZE OF THE HEAP ISSUE e.g. MEMORY LEAK IS COMNIG FROM THIS DUMP");
    // }
    esp32_base_controller::Index(req,res);      
}

void esp32_system_info_controller::prettyFlashModeString(string &flashMode){
    switch (ESP.getFlashChipMode())
    {
    case 0:
        flashMode = "QIO";
        break;
    case 1:
        flashMode = "QOUT";
        break;
    case 2:
        flashMode = "DIO";
        break;
    case 3:
        flashMode = "DOUT";
        break;
    case 4:
        flashMode = "FAST_READ";
        break;
    case 5:
        flashMode = "SLOW_READ";
        break;
    default:
        flashMode = "UNKNOWN";
        break;
    }
}

