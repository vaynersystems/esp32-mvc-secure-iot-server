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
}


void esp32_devices::onLoop()
{
    if(millis() - _lastSnapshotTime < 1000) return;
    //vector<esp32_device_collection_snapshot> snapshot;

    StaticJsonDocument<2048> snapshot;

    
    auto deviceArray = snapshot.to<JsonArray>();
    // get all devices. For each device, read value, store in object
    // if output device, determine desired state. set state if not already set to desired state. 
    /* Read all states */
    /* NOTE: could use arduino json's .is(T) method to test for type instead of storing manually */
    for (int idx = 0; idx < _devices.size(); idx++){        
        int pin = _devices[idx].pin;
        esp32_device_type type = _devices[idx].type;
        JsonObject deviceSeriesEntry = deviceArray.createNestedObject();
        tm now = getDate();
        deviceSeriesEntry["id"] = _devices[idx].id;
        deviceSeriesEntry["time"] = string_format("%02d/%02d/%d %02d:%02d:%02d", now.tm_mon, now.tm_mday, now.tm_year + 1900, now.tm_hour, now.tm_min, now.tm_sec);
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
    //TODO: make configurable parameter
    if(millis() - _lastSnapshotStoreTime > 10000){
        logSnapshot(deviceArray);
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

                auto device = esp32_switch_device(pin);
                auto sourceDeviceState = findDeviceState(deviceArray, _devices[idx].triggerDeviceId);
                auto destinationDeviceState = findDeviceState(deviceArray, _devices[idx].id);

                //switch should maintain the state it had previously
                bool shouldBeOn = getDesiredState(
                    destinationDeviceState["value"].as<bool>(),
                    _devices[idx].triggerType,
                    sourceDeviceState["value"],
                    _devices[idx].triggerValue,
                    sourceDeviceState["type"].as<const char *>()
                );

                //if desired state is different that current state, change it
                if(destinationDeviceState["value"].as<bool>() != shouldBeOn)
                    device.setValue(shouldBeOn); 
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
                auto device = esp32_relay_device(pin);
                auto sourceDeviceState = findDeviceState(deviceArray, _devices[idx].triggerDeviceId);
                auto destinationDeviceState = findDeviceState(deviceArray, _devices[idx].id);
                
                //switch should maintain the state it had previously
                bool shouldBeOn = getDesiredState(
                    destinationDeviceState["value"].as<bool>(),
                    _devices[idx].triggerType,
                    sourceDeviceState["value"],
                    _devices[idx].triggerValue,
                    sourceDeviceState["type"].as<const char *>()
                );

                if(shouldBeOn)
                    device.setValue(true);
                
                device.turnOffIfTime(); //dont do it here, do it in set loop
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
}

// template<typename T>
// inline esp32_base_device<T> *esp32_devices::getDevice(T type)
// {
//     switch(type){
//             case esp32_device_type::AnalogInput:
//                 return esp32_analog_input_device();
//             break;
//             case esp32_device_type::DigitalInput:
//                 return esp32_digital_input_device();
//             break;
//             case esp32_device_type::Thermometer:
            
//             break;
//             case esp32_device_type::Switch:
            
//             break;
//             case esp32_device_type::Relay:
            
//             break;
//             case esp32_device_type::Unknown:
//             default:
//                 return nullptr;
//             break;
//         }
    
// }

JsonObject esp32_devices::findDeviceState(JsonArray deviceStates, int deviceId){
    for(JsonObject deviceState : deviceStates){
        if(!deviceState["id"].isNull() && deviceState["id"].as<int>() == deviceId)
            return deviceState;        
    }
    return JsonObject();
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
        deviceConfig.triggerDeviceId = devicesConfig[idx]["triggerDeviceId"];
        deviceConfig.triggerType = devicesConfig[idx]["triggerType"];
        deviceConfig.triggerValue = devicesConfig[idx]["triggerValue"];
        deviceConfig.type = typeFromTypeName(devicesConfig[idx]["type"]);
        deviceConfig.useTrigger = devicesConfig[idx]["useTrigger"];

        Serial.printf("Adding device to list:\n\tID: \t\t%d\n\tName: \t%s\n\tPin: \t\t%d\n\tDirection: \t%s\n\tHas Trigger: \t%s\n",
            deviceConfig.id,    
            deviceConfig.name.c_str(),    
            deviceConfig.pin,    
            deviceConfig.direction == esp32_device_direction::Input ? "Input" : "Output",
            deviceConfig.useTrigger ? "Yes" : "No"
        );

        _devices.push_back(deviceConfig);
    }

    return _devices;
}

// vector<pair<int, bool>> esp32_devices::GetDeviceStates()
// {
//     vector<pair<int, bool>> deviceStates;
//     auto foundDevices = GetDevices();
//     for(int idx=0; idx< foundDevices.size(); idx++){
//         deviceStates.push_back(pair<int, bool>(foundDevices[idx].id, GetDeviceState<bool>(foundDevices[idx].id)));
//     }
//     return deviceStates;
// }

bool esp32_devices::loadDeviceConfiguration()
{
    return getDevices().size() > 0;
}


// template <typename T>
// T esp32_devices::getDeviceState(int id)
// {
//     auto device = getDevice(id);
//     // according to esp32-hal-gpio, pin state can be read even when its in output mode
//     // so no need to set state.
    
//     auto analogChannel = digitalPinToAnalogChannel(device.pin);
//     if(analogChannel >= 0)
//         return analogRead(device.pin);
    
//     return digitalRead(device.pin);
// }

// template <typename T>
// bool esp32_devices::SetDeviceState(int id, T state)
// {
//     auto device = GetDevice(id);
//     if(device.direction != esp32_device_direction::Output)
//         return false;

//     //set pin mode, write output
//     pinMode(device.pin, OUTPUT);
//     digitalWrite(device.pin, state);
//     return true;
// }

/// @brief For FUCKS SAKE!! the seek function does not work, so we cannot have a properly formatted json array with open and close brackets.
/// @brief Need to work with file directly since loading existing records in log to json doc can overflow (at about 60 entries)
/// @param snapshot 
void esp32_devices::logSnapshot(JsonArray snapshot){
    if(snapshot.isNull() || snapshot.size() == 0) return;
    string snapshotString = "";
    serializeJson(snapshot, snapshotString);
    
    struct tm timeinfo = getDate();
    
    // if(timeinfo.tm_year == 70)
    //     return; // clock not initialized
    string filename = string_format("%s/Devices_%d-%d-%d.log",PATH_LOGGING_ROOT, timeinfo.tm_year + 1900, timeinfo.tm_mon, timeinfo.tm_mday);

    bool logFileExists =  SPIFFS.exists(filename.c_str());

    File snapshotFile = SPIFFS.open(filename.c_str(),"a",true);
    
    if(logFileExists)
        snapshotFile.print(',');
    
    Serial.printf("Adding snapshot data of length %d at position %d to today's snapshots.\n\n\n", snapshotString.length(), snapshotFile.position());
    snapshotFile.print(snapshotString.c_str());
    snapshotFile.println();       
    snapshotFile.close();
    
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