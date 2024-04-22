#include "esp32_wifi.h"

esp32_wifi::esp32_wifi(){
    
}

bool esp32_wifi::start(){
    Serial.println("Setting up WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PSK);
    unsigned long startConnectTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
        if (millis() - startConnectTime > 15000)
        {
            //failed to start in STA mode. Start in AP mode
            IPAddress addr = IPAddress(1, 1, 1, 1);
            WiFi.config(addr, addr, IPAddress(255,255,255,0), addr, addr);
            WiFi.softAP("ESP32_Server", "ESP32_Server", 7, 0, 3);
            break;
        }
    }

  
    
    Serial.print("Connected. IP=");
    Serial.println(WiFi.localIP());

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    return WiFi.status() == WL_CONNECTED;
}