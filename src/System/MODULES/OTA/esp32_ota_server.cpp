#include "esp32_ota_server.hpp"

esp_err_t esp32_ota_server::updateFirmware(const byte *newFirmware, size_t length)
{   
    lcd.setTitle("Firmware update");
    esp_err_t result = ESP_OK;
    result = _startUpdate(length);
    if(result != ESP_OK){
        #if defined(DEBUG_OTA) && DEBUG_OTA > 0
        Serial.printf("Failed to start firmware update %s\n", esp_err_to_name(result));
        #endif
        return result;
    }
    result = _writeFirmwareChunk(newFirmware, length);
    if(result != ESP_OK){
        #if defined(DEBUG_OTA) && DEBUG_OTA > 0
        Serial.printf("Failed to write firmware update %s\n", esp_err_to_name(result));
        #endif
        return result;
    }
    lcd.setDetails("finished writing...");
    #if defined(DEBUG_OTA) && DEBUG_OTA > 0
    Serial.println("Finished writing firmware...");
    #endif
    result =_commitUpdate();
    if(result != ESP_OK){
        #if defined(DEBUG_OTA) && DEBUG_OTA > 0
        Serial.printf("Failed to commit firmware update %s\n", esp_err_to_name(result));
        #endif
        return result;
    }
    
    lcd.setDetails("Update completed");
    return result;
}

esp_err_t esp32_ota_server::updateFirmware(HTTPRequest *request)
{
    lcd.setTitle("Firmware update");
    lcd.setDetails("Starting update");
    #if defined(DEBUG_OTA) && DEBUG_OTA > 0
    Serial.printf("starting firmware update\n");
    #endif
    size_t firmwareSize = request->getContentLength();
    esp_err_t result = ESP_OK;
    byte* buf = new byte[512];
    size_t readLength = 0;
    size_t fieldLength = 0;

    //if psram is present, read into memory and update. otherwise read from steam directly
    if(ESP.getPsramSize() > 0 && ESP.getFreePsram() > firmwareSize){
       
        //store in psram.                
        byte * firmwareBinary = (byte *)ps_malloc(firmwareSize);
        if(firmwareBinary == NULL){
            #if defined(DEBUG_OTA) && DEBUG_OTA > 0
            Serial.printf("Failed to allocate %d bytes of PS RAM for firmware update\n", firmwareSize);
            #endif
            lcd.setDetails("failed to allocate PSRAM");            
            delete[] buf;
            return ESP_ERR_NO_MEM;
        }

        while(true){            
            readLength = request->readBytes((byte*)buf,512); 
            //Serial.printf("Read %d bytes at 0x%06X .. \n", readLength, fieldLength);
            if(readLength <= 0) continue;

            memcpy(&firmwareBinary[fieldLength], buf, readLength);
            fieldLength += readLength;
           
            if(request->requestComplete()) break;
        }        

        if(firmwareSize != fieldLength){
            #if defined(DEBUG_OTA) && DEBUG_OTA > 0
            Serial.printf("Reported %d bytes but buffer filled with %d bytes\n", firmwareSize, fieldLength); 
            #endif
            result = ESP_ERR_INVALID_SIZE;           
        }else{
            result = updateFirmware(firmwareBinary, firmwareSize);
        }
        delete[] firmwareBinary;       
    }
    else {
        #if defined(DEBUG_OTA) && DEBUG_OTA > 0
        Serial.println("PSRAM not found or not enough space. Firmware update directly from stream...");
        #endif
        result = _startUpdate(firmwareSize);
        if(result != ESP_OK){
            #if defined(DEBUG_OTA) && DEBUG_OTA > 0
            Serial.printf("Failed to start firmware update %s\n", esp_err_to_name(result));
            #endif
            request->discardRequestBody();
        } else{

            while(true){            
                readLength = request->readBytes((byte*)buf,512); 
                //Serial.printf("Read %d bytes at 0x%06X .. \n", readLength, fieldLength);
                if(readLength <= 0) continue;
                result = _writeFirmwareChunk(buf, readLength);
                fieldLength += readLength;
            
                if(request->requestComplete()) break;
                if(result != ESP_OK) break;
            }

            if(result != ESP_OK){
                #if defined(DEBUG_OTA) && DEBUG_OTA > 0
                Serial.printf("Failed to write firmware update %s\n", esp_err_to_name(result));
                #endif
            } else{
                lcd.setDetails("finished writing...");
                #if defined(DEBUG_OTA) && DEBUG_OTA > 0
                Serial.println("Finished writing firmware...");
                #endif

                result =_commitUpdate();
                if(result != ESP_OK){
                    #if defined(DEBUG_OTA) && DEBUG_OTA > 0
                    Serial.printf("Failed to commit firmware update %s\n", esp_err_to_name(result));
                    #endif
                }
            }
        }

    }
    lcd.setDetails(result == ESP_OK ? "Update completed" : "Update failed");
    delete[] buf;
    return result;
}

esp_err_t esp32_ota_server::_startUpdate(size_t firmwareSize)
{
    _running = esp_ota_get_running_partition();
    _updatePartition = esp_ota_get_next_update_partition(_running);  
    if(_updatePartition == NULL){
        return ESP_ERR_NOT_SUPPORTED;
    }
    esp_app_desc_t info;
    esp_ota_get_partition_description(_updatePartition, &info);
    


    lcd.setDetails(string_format("Firmware: %d bytes", firmwareSize).c_str());    
    
    if (firmwareSize > 0 && firmwareSize < _updatePartition->size) {
        esp_app_desc_t new_app_info;
        if (firmwareSize > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
            // check current version with downloading
            if (esp_efuse_check_secure_version(new_app_info.secure_version) == false) {
                ESP_LOGE(PROGRAM_TAG, "This a new app can not be downloaded due to a secure version is lower than stored in efuse.");
                return ESP_ERR_INVALID_VERSION;
                
            }
            return esp_ota_begin(_updatePartition, OTA_SIZE_UNKNOWN, &_updateHandle);
        }
        
        
    }  
    
    return ESP_ERR_INVALID_SIZE;
}

esp_err_t esp32_ota_server::_writeFirmwareChunk(const byte *newFirmware, size_t length)
{
    lcd.setDetails(string_format("Writing %d bytes", length).c_str());
    #if defined(DEBUG_OTA) && DEBUG_OTA > 0
    Serial.printf("Received requiest to update firmware with %d bytes\n", length);
    #endif
    return esp_ota_write( _updateHandle, (const void *)newFirmware, length);
}

esp_err_t esp32_ota_server::_commitUpdate()
{
    esp_err_t error = esp_ota_end(_updateHandle);
    if (error != ESP_OK) {
        log_e("OTA failed to end, error: %s", esp_err_to_name(error));
        return error;
    } 
    
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(_updatePartition, &ota_state) == ESP_OK) {
        //if rollback enabled
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // run diagnostic function ...
            bool diagnostic_is_ok = true;
            if (diagnostic_is_ok) {
                ESP_LOGI(PROGRAM_TAG, "Diagnostics completed successfully! Continuing execution ...");
                lcd.setDetails("Completed Sucessfully");
                esp_ota_mark_app_valid_cancel_rollback();
                return ESP_OK;
            } else {
                ESP_LOGE(PROGRAM_TAG, "Diagnostics failed! Start rollback to the previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
                return ESP_ERR_INVALID_RESPONSE;
            }
        }// sucess 
        else if( ota_state == ESP_OTA_IMG_VALID || ota_state == ESP_OTA_IMG_UNDEFINED){
            // esp_app_desc_t appDesc;
            // esp_ota_get_partition_description(updatePartition,&appDesc);
            // Serial.printf("Updated firmware v%s created on %s with IDF %s",
            //     appDesc.version, appDesc.date, appDesc.idf_ver
            // );
            error = esp_ota_set_boot_partition(_updatePartition);
            if (error != ESP_OK) {
                log_e("OTA failed to set boot partition, error: %s", esp_err_to_name(error));
                return error;
            }
            #if defined(DEBUG_OTA) && DEBUG_OTA > 0
            Serial.printf("Completed update. Switched boot partition.\n");
            #endif
            lcd.setDetails("Completed Sucessfully");
            return true;
        } 
        else{
            #if defined(DEBUG_OTA) && DEBUG_OTA > 0
            Serial.printf("Unexpected OTA State: 0x%X\n", ota_state);
            #endif
            return ESP_ERR_INVALID_RESPONSE;
        }
    } else{
        #if defined(DEBUG_OTA) && DEBUG_OTA > 0
        Serial.printf("Error in flashing new firmware\n");
        #endif
        return ESP_ERR_INVALID_RESPONSE;
    }
}

bool diagnostic(){
    return true;
}
    
