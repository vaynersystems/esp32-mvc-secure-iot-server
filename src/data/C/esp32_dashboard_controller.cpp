#include "esp32_dashboard_controller.hpp"
#include "System/ROUTER/esp32_template.h"


DerivedController<esp32_dashboard_controller> esp32_dashboard_controller::reg("esp32_dashboard");

void esp32_dashboard_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    bool isIndexNotImplemented = false;
    auto spiffs_mem = esp32_fileio::getMemoryInfo();
    String freeBytesSPIFFSPretty(""), totalBytesSPIFFSPretty(""), usedBytesSPIFFSPretty("");
    esp32_fileio::PrettyFormat(spiffs_mem.freeBytes, &freeBytesSPIFFSPretty);
    esp32_fileio::PrettyFormat(spiffs_mem.usedBytes, &usedBytesSPIFFSPretty);
    esp32_fileio::PrettyFormat(spiffs_mem.totalBytes, &totalBytesSPIFFSPretty);

    controllerTemplate.SetTemplateVariable("$SPIFFS_MEMORY_FREE", freeBytesSPIFFSPretty.c_str());
    controllerTemplate.SetTemplateVariable("$SPIFFS_MEMORY_USED", usedBytesSPIFFSPretty.c_str());
    controllerTemplate.SetTemplateVariable("$SPIFFS_MEMORY_TOTAL", totalBytesSPIFFSPretty.c_str());
    controllerTemplate.SetTemplateVariable("$SPIFFS_MEMORY_PERCENT_USED", to_string(round(((float)spiffs_mem.usedBytes/(float)spiffs_mem.totalBytes)*100)).c_str());
    
    /* HEAP */
    String freeBytesHEAPSPretty(""), usedBytesHEAPPretty("") , totalBytesHEAPPretty("");
	esp32_fileio::PrettyFormat((size_t)esp_get_free_heap_size(), &freeBytesHEAPSPretty);
	esp32_fileio::PrettyFormat((size_t)ESP.getHeapSize(), &totalBytesHEAPPretty);
    esp32_fileio::PrettyFormat((size_t)ESP.getHeapSize() - esp_get_free_heap_size(), &usedBytesHEAPPretty);

    controllerTemplate.SetTemplateVariable("$HEAP_MEMORY_FREE", freeBytesHEAPSPretty.c_str());
    controllerTemplate.SetTemplateVariable("$HEAP_MEMORY_USED", usedBytesHEAPPretty.c_str());
    controllerTemplate.SetTemplateVariable("$HEAP_MEMORY_TOTAL", totalBytesHEAPPretty.c_str());
    controllerTemplate.SetTemplateVariable("$HEAP_MEMORY_PERCENT_USED", to_string(round(((float)(ESP.getHeapSize() - esp_get_free_heap_size())/(float)ESP.getHeapSize())*100)).c_str());

    /* PSRAM */
    String freeBytesPSRAMPretty(""), usedBytesPSRAMPretty("") , totalBytesPSRAMPretty("");
	esp32_fileio::PrettyFormat(ESP.getFreePsram(), &freeBytesPSRAMPretty);
	esp32_fileio::PrettyFormat(ESP.getPsramSize(), &totalBytesPSRAMPretty);
    esp32_fileio::PrettyFormat(ESP.getPsramSize() - ESP.getFreePsram(), &usedBytesPSRAMPretty);

    controllerTemplate.SetTemplateVariable("$PSRAM_MEMORY_FREE", freeBytesPSRAMPretty.c_str());
    controllerTemplate.SetTemplateVariable("$PSRAM_MEMORY_USED", usedBytesPSRAMPretty.c_str());
    controllerTemplate.SetTemplateVariable("$PSRAM_MEMORY_TOTAL", totalBytesPSRAMPretty.c_str());    
    controllerTemplate.SetTemplateVariable("$PSRAM_MEMORY_PERCENT_USED", to_string(round(((float)(ESP.getPsramSize() - ESP.getFreePsram())/(float)ESP.getPsramSize())*100)).c_str());

    /* SKETCH */
    String freeBytesSKETCHPretty(""), usedBytesSKETCHPretty("") , totalBytesSKETCHPretty("");
	esp32_fileio::PrettyFormat(ESP.getFreeSketchSpace(), &freeBytesSKETCHPretty);
	esp32_fileio::PrettyFormat(ESP.getSketchSize() + ESP.getFreeSketchSpace(), &totalBytesSKETCHPretty);
    esp32_fileio::PrettyFormat(ESP.getSketchSize(), &usedBytesSKETCHPretty);

    controllerTemplate.SetTemplateVariable("$SKETCH_MEMORY_FREE", freeBytesSKETCHPretty.c_str());
    controllerTemplate.SetTemplateVariable("$SKETCH_MEMORY_USED", usedBytesSKETCHPretty.c_str());
    controllerTemplate.SetTemplateVariable("$SKETCH_MEMORY_TOTAL", totalBytesSKETCHPretty.c_str());    
    controllerTemplate.SetTemplateVariable("$SKETCH_MEMORY_PERCENT_USED", to_string(round(((float)ESP.getSketchSize()/(float)(ESP.getSketchSize() + ESP.getFreeSketchSpace()))*100)).c_str());
    
	
    //Print debug message
    //controllerTemplate.PrintDebugMessage(req,res);
}

