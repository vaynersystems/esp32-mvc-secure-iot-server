#include "esp32_dashboard_controller.hpp"
#include "System/ROUTER/esp32_template.h"
#include "string_extensions.h"
#include <nvs.h>

extern const int STACK_SIZE;
DerivedController<esp32_dashboard_controller> esp32_dashboard_controller::reg("esp32_dashboard");

void esp32_dashboard_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    
    auto spiffs_mem = esp32_fileio::getMemoryInfo();
    nvs_stats_t stats;
    nvs_get_stats(NULL, &stats);    

    String flashSizePretty(""), sketchSizePretty(""), flashMode; 
    esp32_fileio::PrettyFormat((size_t)ESP.getFlashChipSize(), &flashSizePretty);
    esp32_fileio::PrettyFormat((size_t)ESP.getSketchSize(), &sketchSizePretty);
    
    prettyFlashModeString(flashMode);

    controllerTemplate.SetTemplateVariable("$_CHIP_CORES",      to_string(ESP.getChipCores()));
    controllerTemplate.SetTemplateVariable("$_CHIP_MODEL",      ESP.getChipModel());
    controllerTemplate.SetTemplateVariable("$_CHIP_REVISION",   to_string(ESP.getChipRevision()));
    controllerTemplate.SetTemplateVariable("$_CPU_FREQUENCY",   to_string(ESP.getCpuFreqMHz()));
    controllerTemplate.SetTemplateVariable("$_FLASH_MODE",      flashMode.c_str());
    controllerTemplate.SetTemplateVariable("$_FLASH_SIZE",      flashSizePretty.c_str());
    controllerTemplate.SetTemplateVariable("$_FLASH_SPEED",     to_string(ESP.getFlashChipSpeed()));
    controllerTemplate.SetTemplateVariable("$_SDK_VERSION",     ESP.getSdkVersion());
    controllerTemplate.SetTemplateVariable("$_SKETCH_SIZE",     sketchSizePretty.c_str()) ;
    



    String freeBytesSPIFFSPretty(""), totalBytesSPIFFSPretty(""), usedBytesSPIFFSPretty("");
    esp32_fileio::PrettyFormat(spiffs_mem.freeBytes, &freeBytesSPIFFSPretty);
    esp32_fileio::PrettyFormat(spiffs_mem.usedBytes, &usedBytesSPIFFSPretty);
    esp32_fileio::PrettyFormat(spiffs_mem.totalBytes, &totalBytesSPIFFSPretty);

    controllerTemplate.SetTemplateVariable("$SPIFFS_MEMORY_FREE", freeBytesSPIFFSPretty.c_str());
    controllerTemplate.SetTemplateVariable("$SPIFFS_MEMORY_USED", usedBytesSPIFFSPretty.c_str());
    controllerTemplate.SetTemplateVariable("$SPIFFS_MEMORY_TOTAL", totalBytesSPIFFSPretty.c_str());
    controllerTemplate.SetTemplateVariable("$SPIFFS_MEMORY_PERCENT_USED", to_string_with_precision(round(((float)spiffs_mem.usedBytes/(float)spiffs_mem.totalBytes)*100),1).c_str());
    controllerTemplate.SetTemplateVariable("$SPIFFS_MEMORY_AVAILABLE", spiffs_mem.totalBytes > 0 ? "block" : "none");
    
    /* HEAP */
    String freeBytesHEAPSPretty(""), usedBytesHEAPPretty("") , totalBytesHEAPPretty("");
	esp32_fileio::PrettyFormat((size_t)esp_get_free_heap_size(), &freeBytesHEAPSPretty);
	esp32_fileio::PrettyFormat((size_t)ESP.getHeapSize(), &totalBytesHEAPPretty);
    esp32_fileio::PrettyFormat((size_t)ESP.getHeapSize() - esp_get_free_heap_size(), &usedBytesHEAPPretty);

    controllerTemplate.SetTemplateVariable("$HEAP_MEMORY_FREE", freeBytesHEAPSPretty.c_str());
    controllerTemplate.SetTemplateVariable("$HEAP_MEMORY_USED", usedBytesHEAPPretty.c_str());
    controllerTemplate.SetTemplateVariable("$HEAP_MEMORY_TOTAL", totalBytesHEAPPretty.c_str());
    controllerTemplate.SetTemplateVariable("$HEAP_MEMORY_PERCENT_USED", to_string_with_precision(round(((float)(ESP.getHeapSize() - esp_get_free_heap_size())/(float)ESP.getHeapSize())*100),1).c_str());
    controllerTemplate.SetTemplateVariable("$HEAP_MEMORY_AVAILABLE", ESP.getHeapSize() > 0 ? "block" : "none");

    /* PSRAM */
    String freeBytesPSRAMPretty(""), usedBytesPSRAMPretty("") , totalBytesPSRAMPretty("");
	esp32_fileio::PrettyFormat(ESP.getFreePsram(), &freeBytesPSRAMPretty);
	esp32_fileio::PrettyFormat(ESP.getPsramSize(), &totalBytesPSRAMPretty);
    esp32_fileio::PrettyFormat(ESP.getPsramSize() - ESP.getFreePsram(), &usedBytesPSRAMPretty);

    controllerTemplate.SetTemplateVariable("$PSRAM_MEMORY_FREE", freeBytesPSRAMPretty.c_str());
    controllerTemplate.SetTemplateVariable("$PSRAM_MEMORY_USED", usedBytesPSRAMPretty.c_str());
    controllerTemplate.SetTemplateVariable("$PSRAM_MEMORY_TOTAL", totalBytesPSRAMPretty.c_str());    
    controllerTemplate.SetTemplateVariable("$PSRAM_MEMORY_PERCENT_USED", to_string_with_precision(round(((float)(ESP.getPsramSize() - ESP.getFreePsram())/(float)ESP.getPsramSize())*100),1).c_str());
    controllerTemplate.SetTemplateVariable("$PSRAM_MEMORY_AVAILABLE", ESP.getPsramSize() > 0 ? "block" : "none");

    /* SKETCH */
    String freeBytesSKETCHPretty(""), usedBytesSKETCHPretty("") , totalBytesSKETCHPretty("");
	esp32_fileio::PrettyFormat(ESP.getFreeSketchSpace(), &freeBytesSKETCHPretty);
	esp32_fileio::PrettyFormat(ESP.getSketchSize() + ESP.getFreeSketchSpace(), &totalBytesSKETCHPretty);
    esp32_fileio::PrettyFormat(ESP.getSketchSize(), &usedBytesSKETCHPretty);

    controllerTemplate.SetTemplateVariable("$SKETCH_MEMORY_FREE", freeBytesSKETCHPretty.c_str());
    controllerTemplate.SetTemplateVariable("$SKETCH_MEMORY_USED", usedBytesSKETCHPretty.c_str());
    controllerTemplate.SetTemplateVariable("$SKETCH_MEMORY_TOTAL", totalBytesSKETCHPretty.c_str());    
    controllerTemplate.SetTemplateVariable("$SKETCH_MEMORY_PERCENT_USED", to_string_with_precision(round(((float)ESP.getSketchSize()/(float)(ESP.getSketchSize() + ESP.getFreeSketchSpace()))*100),1).c_str());
    controllerTemplate.SetTemplateVariable("$SKETCH_MEMORY_AVAILABLE", ESP.getSketchSize() + ESP.getFreeSketchSpace() > 0 ? "block" : "none");
    

    controllerTemplate.SetTemplateVariable("$NVS_FREE_ENTRIES", (to_string_with_precision(stats.free_entries,0) + " bytes").c_str());
    controllerTemplate.SetTemplateVariable("$NVS_USED_ENTRIES", (to_string_with_precision(stats.used_entries,0)+ " bytes").c_str());
    controllerTemplate.SetTemplateVariable("$NVS_TOTAL_ENTRIES", (to_string_with_precision(stats.total_entries,0)+ " bytes").c_str());
    controllerTemplate.SetTemplateVariable("$NVS_PERCENT_USED", (to_string_with_precision(round(((float)stats.used_entries/(float)stats.total_entries)*100),1).c_str()));
	controllerTemplate.SetTemplateVariable("$NVS_MEMORY_AVAILABLE", ESP.getSketchSize() + ESP.getFreeSketchSpace() > 0 ? "block" : "none");
    //Print debug message
    //controllerTemplate.PrintDebugMessage(req,res);
    auto stackFreeBytes = uxTaskGetStackHighWaterMark(NULL);     
    String freeBytesSTACKPretty(""), usedBytesSTACKPretty("") , totalBytesSTACKPretty("");
	esp32_fileio::PrettyFormat(stackFreeBytes, &freeBytesSTACKPretty);
	esp32_fileio::PrettyFormat(STACK_SIZE - stackFreeBytes, &usedBytesSTACKPretty);
    esp32_fileio::PrettyFormat(STACK_SIZE, &totalBytesSTACKPretty);
    controllerTemplate.SetTemplateVariable("$STACK_FREE", freeBytesSTACKPretty.c_str());
    controllerTemplate.SetTemplateVariable("$STACK_USED", usedBytesSTACKPretty.c_str());
    controllerTemplate.SetTemplateVariable("$STACK_TOTAL", totalBytesSTACKPretty.c_str());
    controllerTemplate.SetTemplateVariable("$STACK_PERCENT_USED", (to_string_with_precision(round(((float)(STACK_SIZE - stackFreeBytes)/(float)STACK_SIZE)*100),1).c_str()));    
}

void esp32_dashboard_controller::prettyFlashModeString(String &flashMode){
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

