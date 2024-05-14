#include "esp32_config.h"

    JsonObject esp32_config::getConfig(){
        File f = SPIFFS.open(PATH_SYSTEM_CONFIG,"r");
        StaticJsonDocument<1024> configDoc;
        auto error = deserializeJson(configDoc, f);
        f.close();
        return configDoc.as<JsonObject>();
    }
    bool esp32_config::getConfigSection(const char* sectionName, JsonDocument* loadObject){
        File f = SPIFFS.open(PATH_SYSTEM_CONFIG,"r");
        StaticJsonDocument<2048> configDoc;
        auto error = deserializeJson(configDoc, f);
        f.close();
        if(error.code()  == DeserializationError::Ok){
            //serializeJson(configDoc,Serial);
            loadObject->set(configDoc[sectionName].as<JsonVariant>());   
            return true;
        }
        loadObject->set(configDoc.as<JsonVariant>());
        return false;
    }
    

