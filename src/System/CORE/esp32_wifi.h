#ifndef _ESP32_WIFI_H
#define _ESP32_WIFI_H



#include "../Config.h"
// We will use wifi
#include <WiFi.h>
#include "lwip/apps/sntp.h"
#include <ESPmDNS.h>

#include <SPIFFS.h>
#include "ArduinoJson.h"
#include "esp32_config.hpp"


class esp32_wifi
{
public:
    esp32_wifi();
    bool start();
    // bool startAP();
    // bool startSTA();
    bool end();


private:

    //hw_timer_t *timerWifiDisable = NULL;

};
#endif