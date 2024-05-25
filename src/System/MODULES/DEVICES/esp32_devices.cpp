#include "esp32_devices.hpp"
#include <esp32-hal-gpio.h>


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
                sensors.setResolution(10); //9 bit takes 155ms, 12bit 810ms
                break;

            case esp32_device_type::Switch:
            case esp32_device_type::Relay:
                pinMode(_devices[idx].pin, OUTPUT);
                break;
            case esp32_device_type::Unknown:

                break;
        }       
    }
    #ifdef DEBUG
    Serial.println("Done initializing devices");
    #endif
}


void esp32_devices::onLoop()
{
    //handled by task manager delay
    //if(millis() - _lastSnapshotTime < 500) return;
    //vector<esp32_device_collection_snapshot> snapshot;

    
    
    auto seriesEntry = _scratchpad.to<JsonObject>();
    
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
        bool timeToPublish = mqtt.enabled() && _devices[idx].mqttPublish && millis() - _devices[idx]._lastPublishTime > _devices[idx].mqttFrequency * 60000 ;
        
        switch(type){
            case esp32_device_type::AnalogInput:
            {
                auto device = esp32_analog_input_device(pin);
                auto value = device.getValue();
                deviceSeriesEntry["value"] = value;
                if(timeToPublish){
                    mqtt.publish(_devices[idx].mqttTopic.c_str(),deviceSeriesEntry["value"].as<string>().c_str());
                    _devices[idx]._lastPublishTime = millis();
                }
                
            }
            break;
            case esp32_device_type::DigitalInput:
            {
                auto device = esp32_digital_input_device(pin);
                auto value = device.getValue();
                deviceSeriesEntry["value"] =value;
                if(timeToPublish){
                   mqtt.publish(_devices[idx].mqttTopic.c_str(),deviceSeriesEntry["value"].as<string>().c_str());
                    _devices[idx]._lastPublishTime = millis();
                }
            }
                
            break;
            case esp32_device_type::Thermometer:{
                auto device = esp32_thermometer_device(pin);                               
                auto value = device.getValue();
                deviceSeriesEntry["value"] =value;
                if(timeToPublish){
                    mqtt.publish(_devices[idx].mqttTopic.c_str(),deviceSeriesEntry["value"].as<string>().c_str());
                    _devices[idx]._lastPublishTime = millis();
                }       
            }          
            break;
            case esp32_device_type::Switch:
            {
                auto device = esp32_switch_device(pin);
                deviceSeriesEntry["value"] = device.getValue();
            }            
            break;
            case esp32_device_type::Relay:
            {
                auto device = esp32_relay_device(pin);
                deviceSeriesEntry["value"] = device.getValue();
            }            
            break;
            case esp32_device_type::Unknown:            
                continue;
            break;
        }
    }

    if(millis() - _lastSnapshotStoreTime > _snapshotFrequency){
        logger.logSnapshot(seriesEntry);
        _lastSnapshotStoreTime = millis();
    }
    _snapshot.clear();
    _snapshot.set(seriesEntry);
    
    
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
                auto sourceDeviceStateIdx = findDeviceStateIndex(seriesEntries, _devices[idx].triggerDeviceId);
                auto destinationDeviceIdx = findDeviceStateIndex(seriesEntries, _devices[idx].id);

                if(sourceDeviceStateIdx < 0 || destinationDeviceIdx < 0) 
                    continue; //canot process

                bool currentState = seriesEntries[destinationDeviceIdx]["value"].as<bool>();

                //switch should maintain the state it had previously
                bool shouldBeOn = getDesiredState(
                    seriesEntries[destinationDeviceIdx]["value"].as<bool>(),
                    _devices[idx].triggerType,
                    seriesEntries[sourceDeviceStateIdx]["value"],
                    _devices[idx].triggerValue,
                    _devices[idx].triggerThreshold);

                if(!currentState && shouldBeOn)
                    logger.logInfo(string_format("%s ON at %s", _devices[idx].name.c_str(), date.c_str()).c_str(), esp32_log_type::device);
                else if(currentState && !shouldBeOn)
                    logger.logInfo(string_format("%s OFF at %s", _devices[idx].name.c_str(), date.c_str()).c_str(), esp32_log_type::device);

                //if desired state is different that current state, change it
                if(seriesEntries[destinationDeviceIdx]["value"].as<bool>() != shouldBeOn){
                    device.setValue(shouldBeOn);  
                   if(_devices[idx].mqttPublish && mqtt.enabled())
                        mqtt.publish(_devices[idx].mqttTopic.c_str(), shouldBeOn  ? "On" : "Off");                   
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
                    _devices[idx].triggerThreshold
                );
                if(!currentState && shouldBeOn)
                    logger.logInfo(string_format("%s ON at %s", _devices[idx].name.c_str(), date.c_str()).c_str(), esp32_log_type::device);

                if(shouldBeOn){
                    device.setValue(true);
                    _devices[idx]._lastStartTime = millis();
                }
                
                if(device.turnOffIfTime(_devices[idx].duration)) //dont do it here, do it in set loop
                    logger.logInfo(string_format("%s OFF at %s", _devices[idx].name.c_str(), date.c_str()).c_str(), esp32_log_type::device);
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

bool esp32_devices::getDesiredState(
    bool currentState,
    esp32_device_trigger_type triggerType, 
    JsonVariant value, 
    double triggerValue,
    unsigned long triggerThreshold
){
    bool shouldBeOn = currentState; // switch value is a boolean, safe to cast
    switch (triggerType)
    {
        case esp32_device_trigger_type::LessThan:{
            shouldBeOn = isLessThan(currentState, value, triggerValue, triggerThreshold);
        }
            break;
        case esp32_device_trigger_type::GreaterThan:
            shouldBeOn = isGreaterThan(currentState, value, triggerValue, triggerThreshold);
            break;

        case esp32_device_trigger_type::Equals:
            shouldBeOn = isEqualTo(value, triggerValue);
            break;
        
        default:
            break;
    }
    return shouldBeOn;
}

bool esp32_devices::isLessThan(bool currentState, JsonVariant value, double triggerValue, unsigned long triggerThreshold){
    if(value.is<uint16_t>()){
        uint16_t sourceDeviceValue = value.as<uint16_t>();
        return sourceDeviceValue < (currentState ? triggerValue + triggerThreshold : triggerValue);

    } 
    else if(value.is<double>()){
        double sourceDeviceValue = value.as<double>();
        return sourceDeviceValue < (currentState ? triggerValue + triggerThreshold : triggerValue);

    }
    else if(value.is<bool>()){    
        bool sourceDeviceValue = value.as<bool>();
        return sourceDeviceValue < (currentState ? triggerValue + triggerThreshold : triggerValue);
    }
    Serial.printf("Error occred checking less than condition. value type is unknown");
    return false;
}

bool esp32_devices::isGreaterThan(bool currentState, JsonVariant value, double triggerValue, unsigned long triggerThreshold)
{
    if(value.is<uint16_t>()){
        uint16_t sourceDeviceValue = value.as<uint16_t>();
        return sourceDeviceValue > (currentState ? triggerValue - triggerThreshold : triggerValue);

    } 
    else if(value.is<double>()){
        double sourceDeviceValue = value.as<double>();
        return sourceDeviceValue > (currentState ? triggerValue - triggerThreshold : triggerValue);

    }
    else if(value.is<bool>()){    
        bool sourceDeviceValue = value.as<bool>();
        return sourceDeviceValue > (currentState ? triggerValue - triggerThreshold : triggerValue);
    }
    return false;
}

bool esp32_devices::isEqualTo( JsonVariant value, double triggerValue)
{
    if(value.is<uint16_t>()){
    //if(type == "uint16_t"){
        uint16_t sourceDeviceValue = value.as<uint16_t>();
        return sourceDeviceValue == triggerValue;

    }
    //else if(type == "double"){
    else if(value.is<double>()){
        double sourceDeviceValue = value.as<double>();
        return sourceDeviceValue == triggerValue;

    }
    else if(value.is<bool>()){
    //else if(type == "bool"){}
    
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
        deviceConfig.id = devicesConfig[idx]["id"];
        deviceConfig.name = devicesConfig[idx]["name"].as<string>();
        deviceConfig.pin = devicesConfig[idx]["pin"];
        if(
            !devicesConfig[idx]["trigger"].isNull() && 
            !devicesConfig[idx]["trigger"]["source"].isNull() && 
            !devicesConfig[idx]["trigger"]["type"].isNull()
        ){
            deviceConfig.useTrigger = true;
            deviceConfig.triggerDeviceId = devicesConfig[idx]["trigger"]["source"];
            deviceConfig.triggerType = triggerTypeFromName(devicesConfig[idx]["trigger"]["type"]);
            deviceConfig.triggerValue = devicesConfig[idx]["trigger"]["value"];
            deviceConfig.triggerThreshold = devicesConfig[idx]["trigger"]["threshold"];
        }
        if(!devicesConfig[idx]["mqtt"].isNull()){
            deviceConfig.mqttPublish = !devicesConfig[idx]["mqtt"].isNull() && devicesConfig[idx]["mqtt"]["publish"].as<bool>();
            if(deviceConfig.mqttPublish){
                deviceConfig.mqttTopic = devicesConfig[idx]["mqtt"]["topic"].as<const char*>();
                deviceConfig.mqttFrequency = devicesConfig[idx]["mqtt"]["frequency"].as<int>();
            }
                
        }

        deviceConfig.type = typeFromTypeName(devicesConfig[idx]["type"]);       
        if(deviceConfig.type == Relay)
            deviceConfig.duration =  devicesConfig[idx]["duration"].isNull() ? 5000 : devicesConfig[idx]["duration"].as<int>() * 1000;

        #ifdef DEBUG  
        Serial.printf("Adding device to list:\n\tID: \t\t%d\n\tName: \t\t%s\n\tType: \t\t%s%s\n\tPin: \t\t%d\n\tHas Trigger: \t%s\n",
            deviceConfig.id,    
            deviceConfig.name.c_str(),
            deviceConfig.type == AnalogInput ? "Analog" :
                deviceConfig.type == DigitalInput ? "Digital" :
                deviceConfig.type == Thermometer ? "Thermometer" :
                deviceConfig.type == Switch ? "Switch" : "Relay",
            deviceConfig.type == Relay ? string_format("\n\tDuration: 't%d seconds",deviceConfig.duration / 1000).c_str() : "",
            deviceConfig.pin,    
            deviceConfig.useTrigger ? "Yes" : "No"
        );

        if(deviceConfig.useTrigger){
            Serial.printf("\tTrigger Source: %d\n\tTrigger Type: \t%c\n\tTrigger Value\t%lf\n\tTrigger Threshold:%lu\n",
                deviceConfig.triggerDeviceId,
                deviceConfig.triggerType == LessThan ? '<' : 
                    deviceConfig.triggerType == GreaterThan? '>' : '=',
                deviceConfig.triggerValue,
                deviceConfig.triggerThreshold

            );
        }
        #endif

        _devices.push_back(deviceConfig);
    }

    return _devices;
}

StaticJsonDocument<512> *esp32_devices::getLastSnapshot()
{
    return &_snapshot;
}

// vector<esp32_device_info> esp32_devices::getDeviceConfiguration()
// {
//     return _devices;
// }

bool esp32_devices::loadDeviceConfiguration()
{
    StaticJsonDocument<2048> systemConfig;
    esp32_config::getConfigSection("system", &systemConfig);   
    if(!systemConfig["logging"].isNull()){
        if(!systemConfig["logging"]["frequency"].isNull())
            _snapshotFrequency = systemConfig["logging"]["frequency"].as<unsigned long>() * 1000;
    }
    
    return getDevices().size() > 0;
}

esp32_device_type esp32_devices::typeFromTypeName(const char * typeName){    
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
    return esp32_device_trigger_type::Equals; //default
}
