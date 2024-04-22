#ifndef _ESP32_WIFI_H
#define _ESP32_WIFI_H



#include "../Config.h"
// We will use wifi
#include <WiFi.h>
#include "lwip/apps/sntp.h"

//TODO: move to config file
#define WIFI_SSID "dd-wrt"
#define WIFI_PSK  "4409420820"


class esp32_wifi
{
public:
    esp32_wifi();
    bool start();
    bool startAP();
    bool startSTA();


};
#endif