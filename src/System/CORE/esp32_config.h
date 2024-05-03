#ifndef _ESP32_CONFIG_H
#define _ESP32_CONFIG_H



#include "../Config.h"
#include "ArduinoJson.h"
// We will use wifi
#include <WiFi.h>
#include "lwip/apps/sntp.h"
#include <SPIFFS.h>



class esp32_config
{
public:
    static inline JsonObject getConfig();
    static bool getConfigSection(const char* sectionName, JsonDocument* loadObject);
    


};
#endif