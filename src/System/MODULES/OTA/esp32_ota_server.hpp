#ifndef _ESP32_OTA_SERVER_H
#define _ESP32_OTA_SERVER_H
#include "System/Config.h"
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <esp_ota_ops.h>
#include <esp_efuse.h>

class esp32_ota_server{
    public:
    bool updateFirmware(const byte* newFirmware, size_t length){
        /*
        //Arduino way
        if(!Update.begin(length)){
            Serial.printf("Update failed. Not enough space for new firmware ( %u bytes)\n", length);    
            return false;
        }
        Serial.printf("Writing ( %u bytes)to firmware\n", length);
        
        Update.write((uint8_t*)newFirmware,length);
        if(Update.end(false)){
            Serial.printf("Update completed. Restarting...\n");   
            return true;        
        }
        else{
            Serial.println("Update Failed!");
            return false;
        }
    }  
    */
        //ESP-IDF way
      
        Serial.printf("Received requiest to update firmware with %d bytes\n", length);
        const esp_partition_t *running = esp_ota_get_running_partition();
        const esp_partition_t *updatePartition = esp_ota_get_next_update_partition(running);
        
        esp_ota_handle_t updateHandle ;
        if (length > 0) {
            esp_app_desc_t new_app_info;
            if (length > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                // check current version with downloading
                if (esp_efuse_check_secure_version(new_app_info.secure_version) == false) {
                    ESP_LOGE(PROGRAM_TAG, "This a new app can not be downloaded due to a secure version is lower than stored in efuse.");
                    return false;
                    
                }
                esp_ota_begin(updatePartition, OTA_SIZE_UNKNOWN, &updateHandle);
            }
            
            esp_ota_write( updateHandle, (const void *)newFirmware, length);
        }
        Serial.println("Finished writing firmware...");

        esp_err_t error = esp_ota_end(updateHandle);
        if (error != ESP_OK) {
            log_e("OTA failed to end, error: %s", esp_err_to_name(error));
        } 
        
        esp_ota_img_states_t ota_state;
        if (esp_ota_get_state_partition(updatePartition, &ota_state) == ESP_OK) {
            //if rollback enabled
            if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
                // run diagnostic function ...
                bool diagnostic_is_ok = diagnostic();
                if (diagnostic_is_ok) {
                    ESP_LOGI(PROGRAM_TAG, "Diagnostics completed successfully! Continuing execution ...");
                    esp_ota_mark_app_valid_cancel_rollback();
                    return true;
                } else {
                    ESP_LOGE(PROGRAM_TAG, "Diagnostics failed! Start rollback to the previous version ...");
                    esp_ota_mark_app_invalid_rollback_and_reboot();
                    return false;
                }
            }// sucess 
            else if( ota_state == ESP_OTA_IMG_VALID || ota_state == ESP_OTA_IMG_UNDEFINED){               
               
                error = esp_ota_set_boot_partition(updatePartition);
                if (error != ESP_OK) {
                    log_e("OTA failed to set boot partition, error: %s", esp_err_to_name(error));
                    return false;
                }
                Serial.printf("Completed update. Switched boot partition.\n");
                return true;
            } 
            else{
                Serial.printf("Unexpected OTA State: 0x%X\n", ota_state);
            }
        } else{
            Serial.printf("Error in flashing new firmware\n");
        }
        return false;
    }

    bool diagnostic(){
        return true;
    }
    

};
#endif