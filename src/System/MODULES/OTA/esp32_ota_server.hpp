#ifndef _ESP32_OTA_SERVER_H
#define _ESP32_OTA_SERVER_H
#include "System/Config.h"
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <esp_ota_ops.h>

class esp32_ota_server{
    public:
    bool updateFirmware(byte* newFirmware, size_t length){
        
        const esp_partition_t *running = esp_ota_get_running_partition();
        esp_ota_img_states_t ota_state;
        if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
            if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
                // run diagnostic function ...
                bool diagnostic_is_ok = diagnostic();
                if (diagnostic_is_ok) {
                    ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
                    esp_ota_mark_app_valid_cancel_rollback();
                } else {
                    ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
                    esp_ota_mark_app_invalid_rollback_and_reboot();
                }
            }
        }
    }

    bool diagnostic(){
        return true;
    }

};
#endif;