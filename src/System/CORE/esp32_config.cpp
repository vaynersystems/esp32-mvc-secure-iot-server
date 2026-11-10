#include "esp32_config.hpp"

    JsonObject esp32_config::getConfig(){

        auto drive = filesystem.getDisk(SYSTEM_DRIVE);
        File f = drive->open(PATH_SYSTEM_CONFIG,"r");
        StaticJsonDocument<1024> configDoc;
        auto error = deserializeJson(configDoc, f);
        f.close();
        return configDoc.as<JsonObject>();
    }
    bool esp32_config::getConfigSection(const char* sectionName, JsonDocument* loadObject){
        auto drive = filesystem.getDisk(SYSTEM_DRIVE);
        File f = drive->open(PATH_SYSTEM_CONFIG,"r");
        // Sized from the file: a fixed 2 KB buffer silently failed once the saved config grew,
        // and every section then read as empty (Wi-Fi fell back to access-point mode).
        DynamicJsonDocument configDoc(f.size() * 3 + 1024);
        auto error = deserializeJson(configDoc, f);
        f.close();
        if(error.code()  == DeserializationError::Ok){
            //serializeJson(configDoc,Serial);
            loadObject->set(configDoc[sectionName].as<JsonVariant>());   
            if(loadObject->overflowed())
                Serial.printf("[config] section '%s' is larger than its %u-byte buffer; some settings were dropped\n",
                    sectionName, (unsigned)loadObject->capacity());
            return !loadObject->overflowed();
        }
        Serial.printf("[config] could not read %s: %s\n", PATH_SYSTEM_CONFIG, error.c_str());
        loadObject->clear();
        return false;
    }
    

