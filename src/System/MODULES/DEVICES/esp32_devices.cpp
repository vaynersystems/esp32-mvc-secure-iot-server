#include "esp32_devices.hpp"
#include <esp32-hal-gpio.h>
#include "ArduinoJson.h"
#include <system_helper.h>
#include <string_extensions.h>
void esp32_devices::onInit()
{
    loadDeviceConfiguration();
    for(int idx=0; idx < _devices.size();idx++){
        switch(_devices[idx].type){
            case esp32_device_type::AnalogInput:
            case esp32_device_type::DigitalInput:
                pinMode(_devices[idx].pin, INPUT);

                break;
            case esp32_device_type::Thermometer:
                pinMode(_devices[idx].pin, INPUT_PULLUP);
                //init one wire
                oneWire = OneWire(_devices[idx].pin);
                sensors.setOneWire(&oneWire);
                sensors.begin();
                break;

            case esp32_device_type::Switch:
            case esp32_device_type::Relay:
                pinMode(_devices[idx].pin, OUTPUT);
                break;
            case esp32_device_type::Unknown:

                break;
        }       
    }
    Serial.println("Done initializing devices");
}


void esp32_devices::onLoop()
{
    //handled by task manager delay
    //if(millis() - _lastSnapshotTime < 500) return;
    //vector<esp32_device_collection_snapshot> snapshot;

    StaticJsonDocument<2048> snapshot;
    
    auto seriesEntry = snapshot.createNestedObject();
    
    tm now = getDate();
    auto date = string_format("%02d/%02d/%d %02d:%02d:%02d", now.tm_mon, now.tm_mday, now.tm_year + 1900, now.tm_hour, now.tm_min, now.tm_sec);
    seriesEntry["time"] = date;
    auto seriesEntries = seriesEntry["series"].to<JsonArray>();
    // get all devices. For each device, read value, store in object
    // if output device, determine desired state. set state if not already set to desired state. 

    /* Read all states */
    /* NOTE: could use arduino json's .is(T) method to test for type instead of storing manually */
    for (int idx = 0; idx < _devices.size(); idx++){        
        int pin = _devices[idx].pin;
        esp32_device_type type = _devices[idx].type;
        JsonObject deviceSeriesEntry = seriesEntries.createNestedObject();
        deviceSeriesEntry["id"] = _devices[idx].id;
        // Serial.printf("Getting state of device id %d and type %s..", _devices[idx].id, 
        //     type == esp32_device_type::AnalogInput ? "Analog Input" :
        //     type == esp32_device_type::DigitalInput ? "Digital Input" :
        //     type == esp32_device_type::Thermometer ? "Thermometer" :
        //     type == esp32_device_type::Switch ? "Switch" :
        //     type == esp32_device_type::Relay ? "Relay" : "Unknown"
        // );
        
        switch(type){
            case esp32_device_type::AnalogInput:
            {
                auto device = esp32_analog_input_device(pin);
                deviceSeriesEntry["value"] = device.getValue();
                deviceSeriesEntry["type"] = "uint16_t";
                //Serial.println(deviceSeriesEntry["value"].as<uint16_t>());
            }
            break;
            case esp32_device_type::DigitalInput:
            {
                auto device = esp32_digital_input_device(pin);
                deviceSeriesEntry["value"] = device.getValue();
                deviceSeriesEntry["type"] = "bool";
                //Serial.println(deviceSeriesEntry["value"].as<bool>());
            }
                
            break;
            case esp32_device_type::Thermometer:{
                auto device = esp32_thermometer_device(pin);                               
                deviceSeriesEntry["value"] = device.getValue();
                deviceSeriesEntry["type"] = "double";   
                //Serial.println(deviceSeriesEntry["value"].as<double>());           
            }          
            break;
            case esp32_device_type::Switch:
            {
                auto device = esp32_switch_device(pin);
                deviceSeriesEntry["value"] = device.getValue();
                deviceSeriesEntry["type"] = "bool";
                //Serial.println(deviceSeriesEntry["value"].as<bool>());
            }            
            break;
            case esp32_device_type::Relay:
            {
                auto device = esp32_relay_device(pin);
                deviceSeriesEntry["value"] = device.getValue();
                deviceSeriesEntry["type"] = "bool";
                //Serial.println(deviceSeriesEntry["value"].as<bool>());
            }            
            break;
            case esp32_device_type::Unknown:            
                continue;
            break;
        }
    }
    // Serial.printf("Checking if time passed (%u ms) is greater than configured frequency (%u ms)", 
    //    millis() - _lastSnapshotStoreTime,
    //     _snapshotFrequency
    // );
    if(millis() - _lastSnapshotStoreTime > _snapshotFrequency){
        logSnapshot(seriesEntry);
        _lastSnapshotStoreTime = millis();
    }
    
    
    /* Write output states */
    for (int idx = 0; idx < _devices.size(); idx++){        
        int pin = _devices[idx].pin;        
        switch(_devices[idx].type){            
           
            case esp32_device_type::Switch:
            {
                //determine state
                if(!_devices[idx].useTrigger)
                    continue; //until scheduling is implemented, nothing to do for output device without trigger

                // Serial.printf("Testing switch device for trigger %c %lf\n",
                //     _devices[idx].triggerType == LessThan ? '<' : 
                //         _devices[idx].triggerType == GreaterThan? '>' : '=',
                //     _devices[idx].triggerValue);
                auto device = esp32_switch_device(pin);
                auto sourceDeviceStateIdx = findDeviceStateIndex(seriesEntries, _devices[idx].triggerDeviceId);
                auto destinationDeviceIdx = findDeviceStateIndex(seriesEntries, _devices[idx].id);

                if(sourceDeviceStateIdx < 0 || destinationDeviceIdx < 0) 
                    continue; //canot process

                bool currentState = seriesEntries[destinationDeviceIdx]["value"].as<bool>();

                // Serial.printf("Source Device Idx: %d, Destination Device Idx: %d\n", sourceDeviceStateIdx, destinationDeviceIdx);
                // Serial.printf("Device Config:\n  ID: %d\n  Name: %s\n  Pin: %d\n  Trigger ID: %d\n  Trigger Value: %lf\n",
                //     _devices[idx].id,
                //     _devices[idx].name.c_str(),
                //     _devices[idx].pin,
                //     _devices[idx].triggerDeviceId,
                //     _devices[idx].triggerValue
                // );

                //switch should maintain the state it had previously
                bool shouldBeOn = getDesiredState(
                    seriesEntries[destinationDeviceIdx]["value"].as<bool>(),
                    _devices[idx].triggerType,
                    seriesEntries[sourceDeviceStateIdx]["value"],
                    _devices[idx].triggerValue,
                    seriesEntries[sourceDeviceStateIdx]["type"].as<const char *>()
                );

                if(!currentState && shouldBeOn)
                    Serial.printf("Turning %s ON at %s\n", _devices[idx].name.c_str(), date.c_str());
                else if(currentState && !shouldBeOn)
                    Serial.printf("Turning %s OFF at %s\n", _devices[idx].name.c_str(), date.c_str());


                // Serial.printf(" Setting state to %s based on state from source device %d with value %f\n",
                //     shouldBeOn ? "On" : "Off",
                //     seriesEntries[sourceDeviceStateIdx]["id"].as<int>(),
                //     seriesEntries[sourceDeviceStateIdx]["value"].as<double>() 
                // );

                //if desired state is different that current state, change it
                if(seriesEntries[destinationDeviceIdx]["value"].as<bool>() != shouldBeOn){
                    device.setValue(shouldBeOn);                     
                }
            }            
            break;
            case esp32_device_type::Relay:
            {
                //if relay is on:
                //   1. we will want to reset the timer if the state says it should be on.
                //   2. otherwise we check if it should be turned off (timed out)
                //if relay is off:
                //   1.  we will turn it on (same as "on 1.") is desired state is to turn on
                //   2.  otherwise do nothing
                auto device = esp32_relay_device(pin, _devices[idx]._lastStartTime);
                auto sourceDeviceStateIdx = findDeviceStateIndex(seriesEntries, _devices[idx].triggerDeviceId);
                auto destinationDeviceIdx = findDeviceStateIndex(seriesEntries, _devices[idx].id);
                
                if(sourceDeviceStateIdx < 0 || destinationDeviceIdx < 0) 
                    continue; //canot process
                //switch should maintain the state it had previously
                bool currentState = seriesEntries[destinationDeviceIdx]["value"].as<bool>();
                bool shouldBeOn = getDesiredState(
                    currentState,
                    _devices[idx].triggerType,
                    seriesEntries[sourceDeviceStateIdx]["value"],
                    _devices[idx].triggerValue,
                    seriesEntries[sourceDeviceStateIdx]["type"].as<const char *>()
                );
                if(!currentState && shouldBeOn)
                    Serial.printf("Turning %s ON at %s\n", _devices[idx].name.c_str(), date.c_str());

                if(shouldBeOn){
                    device.setValue(true);
                    _devices[idx]._lastStartTime = millis();
                }
                
                if(device.turnOffIfTime(_devices[idx].duration)) //dont do it here, do it in set loop
                    Serial.printf("Turned %s OFF at %s\n", _devices[idx].name.c_str(), date.c_str());
            }            
            break;
            //no output actions for input devices
            case esp32_device_type::AnalogInput:
            case esp32_device_type::DigitalInput:
            case esp32_device_type::Thermometer:
            case esp32_device_type::Unknown:            
                continue;
            break;
        }
    }
    _lastSnapshotTime = millis();
}

bool esp32_devices::getDesiredState(bool currentState,esp32_device_trigger_type triggerType, JsonVariant value, double triggerValue, const char * valueType){
    bool shouldBeOn = currentState; // switch value is a boolean, safe to cast
    switch (triggerType)
    {
        case esp32_device_trigger_type::LessThan:{
            shouldBeOn = isLessThan(value, triggerValue, valueType);
        }
            break;
        case esp32_device_trigger_type::GreaterThan:
            shouldBeOn = isGreaterThan(value, triggerValue, valueType);
            break;

        case esp32_device_trigger_type::Equals:
            shouldBeOn = isEqualTo(value, triggerValue, valueType);
            break;
        
        default:
            break;
    }
    return shouldBeOn;
}

bool esp32_devices::isLessThan(JsonVariant value, double triggerValue, const char * type){
    if(type == "uint16_t"){
        uint16_t sourceDeviceValue = value.as<uint16_t>();
        return sourceDeviceValue < triggerValue;

    } 
    else if(type == "double"){
        double sourceDeviceValue = value.as<double>();
        return sourceDeviceValue < triggerValue;

    }
    else if(type == "bool")
    {
        bool sourceDeviceValue = value.as<bool>();
        return sourceDeviceValue < triggerValue;
    }

    return false;
}

bool esp32_devices::isGreaterThan(JsonVariant value, double triggerValue, const char *type)
{
    if(type == "uint16_t"){
        uint16_t sourceDeviceValue = value.as<uint16_t>();
        return sourceDeviceValue > triggerValue;

    } 
    else if(type == "double"){
        double sourceDeviceValue = value.as<double>();
        return sourceDeviceValue > triggerValue;

    }
    else if(type == "bool")
    {
        bool sourceDeviceValue = value.as<bool>();
        return sourceDeviceValue > triggerValue;
    }
    return false;
}

bool esp32_devices::isEqualTo(JsonVariant value, double triggerValue, const char *type)
{
    if(type == "uint16_t"){
        uint16_t sourceDeviceValue = value.as<uint16_t>();
        return sourceDeviceValue == triggerValue;

    }
    else if(type == "double"){
        double sourceDeviceValue = value.as<double>();
        return sourceDeviceValue == triggerValue;

    }
    else if(type == "bool")
    {
        bool sourceDeviceValue = value.as<bool>();
        return sourceDeviceValue == triggerValue;
    }
    return false;
}

JsonObject esp32_devices::findDeviceState(JsonArray deviceStates, int deviceId){
    for(JsonObject deviceState : deviceStates){
        if(!deviceState["id"].isNull() && deviceState["id"].as<int>() == deviceId){            
            return deviceState;        
        }
            
    }
    Serial.println("Not found");
    return JsonObject();
}

int esp32_devices::findDeviceStateIndex(JsonArray deviceStates, int deviceId)
{
    int idx = 0;
    for(JsonObject deviceState : deviceStates){
        if(!deviceState["id"].isNull() && deviceState["id"].as<int>() == deviceId){            
            return idx;        
        }
        idx++;
            
    }
    
    return -1;
}

vector<esp32_device_info> esp32_devices::getDevices()
{
    //vector<esp32_device_info> devices;
    StaticJsonDocument<2048> devicesConfig;
    esp32_config::getConfigSection("devices", &devicesConfig);
    _devices.clear();

    for(int idx = 0; idx < devicesConfig.size();idx++){
        esp32_device_info deviceConfig;
        deviceConfig.direction = devicesConfig[idx]["direction"];
        deviceConfig.id = devicesConfig[idx]["id"];
        deviceConfig.name = devicesConfig[idx]["name"].as<string>();
        deviceConfig.pin = devicesConfig[idx]["pin"];
        if(!devicesConfig[idx]["trigger"].isNull()){
            deviceConfig.useTrigger = true;
            deviceConfig.triggerDeviceId = devicesConfig[idx]["trigger"]["source"];
            deviceConfig.triggerType = triggerTypeFromName(devicesConfig[idx]["trigger"]["type"]);
            deviceConfig.triggerValue = devicesConfig[idx]["trigger"]["value"];
        }
            
        deviceConfig.type = typeFromTypeName(devicesConfig[idx]["type"]);       
        if(deviceConfig.type == Relay)
            deviceConfig.duration =  devicesConfig[idx]["duration"].isNull() ? 5000 : devicesConfig[idx]["duration"].as<int>() * 1000;

        Serial.printf("Adding device to list:\n\tID: \t\t%d\n\t\tName: \t%s\n\tType: \t\t%s%s\n\tPin: \t\t%d\n\tDirection: \t%s\n\tHas Trigger: \t%s\n",
            deviceConfig.id,    
            deviceConfig.name.c_str(),
            deviceConfig.type == AnalogInput ? "Analog" :
                deviceConfig.type == DigitalInput ? "Digital" :
                deviceConfig.type == Thermometer ? "Thermometer" :
                deviceConfig.type == Switch ? "Switch" : "Relay",
            deviceConfig.type == Relay ? string_format("\n\tDuration: 't%d seconds",deviceConfig.duration / 1000).c_str() : "",
            deviceConfig.pin,    
            deviceConfig.direction == esp32_device_direction::Input ? "Input" : "Output",
            deviceConfig.useTrigger ? "Yes" : "No"
        );

        
        _devices.push_back(deviceConfig);
    }

    return _devices;
}

bool esp32_devices::loadDeviceConfiguration()
{
    StaticJsonDocument<2048> systemConfig;
    esp32_config::getConfigSection("system", &systemConfig);   
    if(!systemConfig["logging"].isNull()){
        if(!systemConfig["logging"]["frequency"].isNull())
            _snapshotFrequency = systemConfig["logging"]["frequency"].as<unsigned long>() * 1000;       

        if(!systemConfig["logging"]["retention"].isNull() )
        _retentionDays = systemConfig["logging"]["retention"].as<int>();
    }
    
    return getDevices().size() > 0;
}

void esp32_devices::removeOldLogs()
{
    //enuerate logs directory, remove files older than retention period (in days)
    if(!SPIFFS.exists(PATH_LOGGING_ROOT)) return;
    File logsDir = SPIFFS.open(PATH_LOGGING_ROOT);
    if(!logsDir.isDirectory()) return;
    Serial.printf("Removing files older than %d days\n", _retentionDays);
    tm now = getDate(); 
    time_t secondsNow = mktime(&now);
    File file = logsDir.openNextFile();
    while (file) {
        if (file.isDirectory()) { //we're done with logs files
            return;
        }
        else {
            time_t lastWrite = file.getLastWrite();
            if(secondsNow - lastWrite > _retentionDays * 24 * 60 * 60){
                Serial.printf("Removing log file %s due to retention policy.\n", file.name());
            } else {
                Serial.printf("Keeping log file %s since it is %d days old", file.name(), (secondsNow - lastWrite)/(60*60*24));
            }            
        }
        file = logsDir.openNextFile();
        //esp_task_wdt_reset();
    }
}

/// @brief Create or append this frame to the daily snapshot file
/// @param snapshot
void esp32_devices::logSnapshot(JsonObject snapshot){
    if(snapshot.isNull() || snapshot.size() == 0) return;
    string snapshotString = "";
    serializeJson(snapshot, snapshotString);
    
    struct tm timeinfo = getDate();
    
    if(timeinfo.tm_year == 70)
        return; // clock not initialized
    string filename = string_format("%s/Devices_%04d-%02d-%02d.log",PATH_LOGGING_ROOT, timeinfo.tm_year + 1900, timeinfo.tm_mon, timeinfo.tm_mday);

    bool logFileExists =  SPIFFS.exists(filename.c_str());
  
    if(!logFileExists){ //new log file
        //run cleanup
        removeOldLogs();
        File snapshotFile = SPIFFS.open(filename.c_str(),"w",true);
        snapshotFile.print("[\n\t ");
        snapshotFile.print(snapshotString.c_str());
        snapshotFile.println();
        snapshotFile.print(']');
        Serial.printf("Saved first record to snapshot file %s\n", filename.c_str());
        snapshotFile.close();
    } else{
        File snapshotFile = SPIFFS.open(filename.c_str(),"r+w",true);
        int fileSize = snapshotFile.size();
        int seekPos = fileSize - 2;
        //Serial.printf("Seeking from position %d to position %d of %d in daily snapshot file .\n", snapshotFile.position(), seekPos, fileSize);
        bool seekWorked = snapshotFile.seek(seekPos, SeekMode::SeekSet);
        if(!seekWorked){
            #ifdef DEBUG
            Serial.println("Logging - Failed to seek. Aborting!");
            #endif
            return;
        }
        snapshotFile.print("\t,");
        
        // #ifdef DEBUG
        // Serial.printf("Adding snapshot data of length %d at position %d to today's snapshots.\n\n\n", snapshotString.length(), snapshotFile.position(), snapshotString.c_str());
        // #endif
        snapshotFile.print(snapshotString.c_str());
        snapshotFile.println();
        snapshotFile.print(']');
    }
    
    return;
}

esp32_device_type esp32_devices::typeFromTypeName(const char * typeName){
    Serial.printf("Getting device type from name %s\n", typeName);
    if(strcmp(typeName, "AnalogInput") == 0)
        return esp32_device_type::AnalogInput ;
    if(strcmp(typeName, "DigitalInput") == 0)
        return esp32_device_type::DigitalInput ;
    if(strcmp(typeName, "Thermometer") == 0)
        return esp32_device_type::Thermometer ;
    if(strcmp(typeName, "Switch") == 0)
        return esp32_device_type::Switch ;
    if(strcmp(typeName, "Relay") == 0)
        return esp32_device_type::Relay;
    return esp32_device_type::Unknown;
}

esp32_device_trigger_type esp32_devices::triggerTypeFromName(const char *triggerTypeName)
{
    if(strcmp(triggerTypeName,"<") == 0)
        return  LessThan;
    if(strcmp(triggerTypeName,"=") == 0)
        return  Equals;
    if(strcmp(triggerTypeName,">") == 0)
        return  GreaterThan;
}
