#ifndef _ESP32_OTA_SERVER_H
#define _ESP32_OTA_SERVER_H

#include "System/Config.h"
#include "System/MODULES/LCD/esp32_lcd.hpp"
#include "string_helper.h"
#include "HTTPRequest.hpp"
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <esp_ota_ops.h>
#include <esp_efuse.h>

using namespace httpsserver;
extern esp32_lcd lcd;

class esp32_ota_server{

    public:
    esp_err_t updateFirmware(const byte* newFirmware, size_t length);
    esp_err_t updateFirmware(HTTPRequest *request);

    private:
    const esp_partition_t * _running;
    const esp_partition_t * _updatePartition;    
    esp_ota_handle_t _updateHandle ;
   
    esp_err_t _startUpdate(size_t firmwareSize);
    esp_err_t _writeFirmwareChunk(const byte* newFirmware, size_t length);
    esp_err_t _commitUpdate();    

};
#endif