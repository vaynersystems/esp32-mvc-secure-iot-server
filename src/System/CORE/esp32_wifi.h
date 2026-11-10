#ifndef _ESP32_WIFI_H
#define _ESP32_WIFI_H



#include "../Config.h"
// We will use wifi
#include <WiFi.h>
#include "lwip/apps/sntp.h"
#include <ESPmDNS.h>
#include <string>

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

    // Captive portal: while the device runs its own access point, every DNS name resolves
    // to the device and any request for a foreign host is redirected to the login page,
    // so a phone that joins shows its "sign in to network" prompt.
    static bool captivePortalActive();
    // True when the Host header names some other site (e.g. an OS connectivity probe).
    static bool isForeignHost(const std::string& hostHeader);
    static std::string captivePortalUrl(); // "http://<ap ip>/"
    static std::string captivePortalIp();

private:
    static void dnsTask(void* params);
    static int _dnsSocket;
    static bool _captivePortal;
    static bool _logDns;
    static std::string _apIp;
    static std::string _hostname;

    //hw_timer_t *timerWifiDisable = NULL;

};
#endif