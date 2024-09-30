//Notes:   This isa hacky approach because I couldn't get generic list of devices that supports different types of storage for reads and writes.
//         someone with a stronger c/c++ background can likely resolve this with little effort.
#include "esp32_devices.hpp"
#include <esp32-hal-gpio.h>


void esp32_devices::onInit()
{
    #ifdef DEBUG_DEVICE
    Serial.println(F("Initializing device manager"));
    #endif
    loadDeviceConfiguration();
    auto devicePins = pinManager.getControllerPins();
    bool initializedPin = false;
    for(int idx=0; idx < _devices.size();idx++){
        #ifdef DEBUG_DEVICE
        Serial.printf("Initializing device #%d: %s\n", idx + 1, _devices[idx].name.c_str() );
        #endif
        initializedPin = false;
        switch(_devices[idx].type){
            case esp32_device_type::AnalogInput:
            case esp32_device_type::DigitalInput:
                pinMode(_devices[idx].pin, INPUT);

                break;
            case esp32_device_type::Thermometer:
                pinMode(_devices[idx].pin, INPUT);
                //init one wire
                oneWire = OneWire(_devices[idx].pin);
                sensors.setOneWire(&oneWire);                
                sensors.setPullupPin(_devices[idx].pin);
                sensors.begin();
                 //9 bit takes 155ms, 12bit 810ms. default to 10 bit, giving ~0.45F resolution
                sensors.setResolution(_devices[idx].resolution > 0 ? _devices[idx].resolution  : 10);
                break;

            case esp32_device_type::Switch:
            case esp32_device_type::Relay:
                for(int pinIdx = 0; pinIdx < devicePins.size();pinIdx++){
                    if(devicePins[pinIdx].gpioPin == _devices[idx].pin){
                        //pin exists on controller
                        if(devicePins[pinIdx].pinMode == GPIO_MODE_INPUT){
                            Serial.printf("Configured pin %d for device %s is set to output, but controller does not support output on this pin\n",
                                devicePins[pinIdx].gpioPin, _devices[idx].name.c_str()
                            );                            
                        } else{
                            Serial.printf("Configuring pin %d for device %s to output\n", devicePins[pinIdx].gpioPin, _devices[idx].name.c_str());
                            pinMode(_devices[idx].pin, OUTPUT);
                            if(_devices[idx].signal != activeHigh){
                                digitalWrite(_devices[idx].pin, HIGH);
                            }
                            initializedPin = true;
                        }
                        break;
                    }
                }
                if(!initializedPin)
                    Serial.printf("[esp32_devices] ERROR: Output device %s not intilaized! Pin %d not found in controller. Check your device configuration!\n",
                        _devices[idx].name.c_str(), _devices[idx].pin
                    );
                
                break;
            case esp32_device_type::Unknown:
                Serial.printf("esp32_devices] ERROR: Device %s not intilaized! Device type is unknown. Check your device configuration!\n",
                    _devices[idx].name.c_str(), _devices[idx].pin
                );
                break;
        }       
    }
    #ifdef DEBUG
    Serial.println(F("Done initializing devices"));
    #endif
}


void esp32_devices::onLoop()
{   
    auto seriesEntry = _scratchpad.to<JsonObject>();
    
    tm now = getDate();
    auto date = string_format("%02d/%02d/%d %02d:%02d:%02d", now.tm_mon + 1, now.tm_mday, now.tm_year + 1900, now.tm_hour, now.tm_min, now.tm_sec);
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
        bool timeToPublish = mqtt.enabled() && _devices[idx].mqttPublish && millis() - _devices[idx].lastPublishTime > _devices[idx].mqttFrequency * 60000 ;
        
        deviceSeriesEntry["id"] = _devices[idx].id;
        getDeviceState( _devices[idx].id, deviceSeriesEntry);        

        switch(type){
            case esp32_device_type::AnalogInput:
            case esp32_device_type::DigitalInput:
            case esp32_device_type::Thermometer:
            {
                if(timeToPublish){
                    mqtt.publish(_devices[idx].mqttTopic.c_str(),deviceSeriesEntry["value"].as<string>().c_str());
                    _devices[idx].lastPublishTime = millis();
                }
                
            }       
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
        auto device = _devices[idx]; 
        if(_devices[idx].managedByScheduler) continue; //if device is managed by system scheduler, do not manually set state.   
        int pin = _devices[idx].pin;    
        if(_devices[idx].type != Switch && _devices[idx].type != Relay) continue;
        //determine state
        if(!_devices[idx].useTrigger)
            continue; //until scheduling is implemented, nothing to do for output device without trigger
        
        auto sourceDeviceStateIdx = findDeviceStateIndex(seriesEntries, _devices[idx].triggerDeviceId);
        auto destinationDeviceIdx = findDeviceStateIndex(seriesEntries, _devices[idx].id);

        if(sourceDeviceStateIdx < 0 || destinationDeviceIdx < 0) 
            continue; //canot process

        bool electricCurrentState = seriesEntries[destinationDeviceIdx]["value"].as<bool>();
        bool currentState = _devices[idx].signal == activeLow ? !electricCurrentState : electricCurrentState;
        
        if(_devices[idx].signal == activeLow ){
            seriesEntries[destinationDeviceIdx]["value"] = !seriesEntries[destinationDeviceIdx]["value"].as<bool>();
        }

        #ifdef DEBUG_DEVICE
        Serial.printf("Main device loop. Checking device %s state\n", _devices[idx].name.c_str());
        #endif
        //switch should maintain the state it had previously
        bool shouldBeOn = getDesiredState(
            currentState,
            _devices[idx].triggerType,
            seriesEntries[sourceDeviceStateIdx]["value"],
            _devices[idx].triggerValue,
            _devices[idx].triggerThreshold);

        bool electricShouldBeOn = _devices[idx].signal == activeLow ? !shouldBeOn : shouldBeOn;

        #ifdef DEBUG_DEVICE
        Serial.printf("Main device loop. Checking device %s state\n\tActive %s\n\tElectric Is On: %s\n\tIs On: %s\n\tShould Be On: %s\n\tElectric Should Be On: %s\n",
            _devices[idx].name.c_str(), 
            _devices[idx].signal == esp32_device_signal::activeHigh ? "HIGH" : "LOW",
            electricCurrentState ? "ON" : "OFF", currentState ? "ON" : "OFF", shouldBeOn ? "ON" : "OFF", electricShouldBeOn ? "ON" : "OFF");
        #endif

        // if(!currentState && shouldBeOn)
        //     logger.logInfo(string_format("%s ON at %s", _devices[idx].name.c_str(), date.c_str()).c_str(), esp32_log_type::device);
        // else if(currentState && !shouldBeOn)
        //     logger.logInfo(string_format("%s OFF at %s", _devices[idx].name.c_str(), date.c_str()).c_str(), esp32_log_type::device);  

        switch(_devices[idx].type){            
           
            case esp32_device_type::Switch:            
            {
                

                auto device = esp32_switch_device(pin);
                //if desired state is different that current state, change it
                if(currentState != shouldBeOn){
                    
                    //Serial.printf("Setting device %s with state %s to %s\n", _devices[idx].name.c_str(), currentState ? "on" : "off", shouldBeOn ? "on" : "off");
                    device.setValue(electricShouldBeOn);  
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
                
                auto device = esp32_relay_device(pin, _devices[idx].lastStartTime);
                if(shouldBeOn){
                    device.setValue(electricShouldBeOn);
                    _devices[idx].lastStartTime = millis();
                }
                
                if(device.turnOffIfTime(_devices[idx].duration))
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
    if(value.isNull()){
        Serial.printf("Value is null!!\n");
        return false;
    }
    if(value.is<uint16_t>()){
        uint16_t sourceDeviceValue = value.as<uint16_t>();
        return sourceDeviceValue < (currentState ? triggerValue + triggerThreshold : triggerValue);

    } 
    else if(value.is<double>()){
        double sourceDeviceValue = value.as<double>();
        return sourceDeviceValue < (currentState ? triggerValue + triggerThreshold : triggerValue);

    }
    else if(value.is<float>()){
        float sourceDeviceValue = value.as<float>();
        return sourceDeviceValue < (currentState ? triggerValue + triggerThreshold : triggerValue);

    }
    else if(value.is<bool>()){    
        bool sourceDeviceValue = value.as<bool>();
        return sourceDeviceValue < (currentState ? triggerValue + triggerThreshold : triggerValue);
    }
    else if(value.is<int>()){
        double sourceDeviceValue = value.as<int>();
        return sourceDeviceValue < (currentState ? triggerValue + triggerThreshold : triggerValue);
    }
    else if(value.is<short>()){
        double sourceDeviceValue = value.as<int>();
        return sourceDeviceValue < (currentState ? triggerValue + triggerThreshold : triggerValue);
    }
    else if(value.is<const char *>()) {
        Serial.printf("Error occured checking less than condition. value type is const char * %d\n", value.as<const char *>());
        double sourceDeviceValue = value.as<int>();
        return sourceDeviceValue < (currentState ? triggerValue + triggerThreshold : triggerValue);
    }
    else {
        Serial.printf("Error occured checking less than condition. value type is unknown %d : %s\n", value.as<int>(),  value.as<const char *>());
        double sourceDeviceValue = value.as<int>();
        return sourceDeviceValue < (currentState ? triggerValue + triggerThreshold : triggerValue);
    }
    
    return false;
}

bool esp32_devices::isGreaterThan(bool currentState, JsonVariant value, double triggerValue, unsigned long triggerThreshold)
{
    if(value.isNull()){
        Serial.printf("Value is null!!\n");
        return false;
    }
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
    else {
        Serial.printf("Error occured checking greater than condition. value type is unknown %d : %s\n", value.as<int>(),  value.as<const char *>());
        double sourceDeviceValue = value.as<int>();
        return sourceDeviceValue < (currentState ? triggerValue + triggerThreshold : triggerValue);
    }
    return false;
}

bool esp32_devices::isEqualTo( JsonVariant value, double triggerValue)
{
    if(value.isNull()){
        Serial.printf("Value is null!!\n");
        return false;
    }
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
    else {
        Serial.printf("Error occured checking equal to condition. value type is unknown %d : %s\n", value.as<int>(),  value.as<const char *>());
        double sourceDeviceValue = value.as<int>();
        return sourceDeviceValue ==triggerValue;
    }
    return false;
}

JsonObject esp32_devices::findDeviceState(JsonArray deviceStates, int deviceId){
    for(JsonObject deviceState : deviceStates){
        if(!deviceState["id"].isNull() && deviceState["id"].as<int>() == deviceId){            
            return deviceState;        
        }
            
    }
    #ifdef DEBUG
    Serial.println("Not found");
    #endif
    return JsonObject();
}

int esp32_devices::findDeviceStateIndex(JsonArray deviceStates, int deviceId)
{
    int idx = 0;
    for(JsonObject deviceState : deviceStates){
        if(!deviceState["id"].isNull() && deviceState["id"].as<int>() == deviceId){      
            #ifdef DEBUG_DEVICE
                #if DEBUG_DEVICE > 1
                Serial.printf("Found device state idx %d for device idx %d\n", idx, deviceId);
                #endif      
            #endif
            return idx;        
        }
        idx++;
            
    }
    
    return -1;
}

void esp32_devices::getDeviceState(int deviceId, JsonObject object){
    
    for(int deviceIdx = 0; deviceIdx < _devices.size(); deviceIdx++){
        if(_devices[deviceIdx].id == deviceId){
            switch(_devices[deviceIdx].type){
                case esp32_device_type::AnalogInput:
                {
                    auto device = esp32_analog_input_device(_devices[deviceIdx].pin);
                    object["value"] = device.getValue();
                    //return returnValue;                
                    
                }
                break;
                case esp32_device_type::DigitalInput:
                {
                    auto device = esp32_digital_input_device(_devices[deviceIdx].pin);
                    object["value"] = device.getValue();
                    //return returnValue; 
                }
                    
                break;
                case esp32_device_type::Thermometer:{
                    auto device = esp32_thermometer_device(_devices[deviceIdx].pin);                               
                    object["value"] = device.getValue();
                    //return returnValue;      
                }          
                break;
                case esp32_device_type::Switch:
                {
                    auto device = esp32_switch_device(_devices[deviceIdx].pin);
                    object["value"] = device.getValue();
                    //return returnValue; 
                }            
                break;
                case esp32_device_type::Relay:
                {
                    auto device = esp32_relay_device(_devices[deviceIdx].pin);
                    object["value"] = device.getValue();
                    //return returnValue; 
                }            
                break;
                case esp32_device_type::Unknown:            
                    continue;
                break;
            }
            break;
        }
    }
    
    // if(!_snapshot.isNull()){
    //     for(int idx = 0; idx < _snapshot.size(); idx++){
    //         if(_snapshot[idx]["id"].as<int>() == deviceId )
    //         return _snapshot[idx]["id"]["value"];
    //     }
    // }
}

bool esp32_devices::setDeviceState(int deviceId, bool value){
    bool set = false;
    for(int idx = 0; idx < _devices.size();idx++)
        if(_devices[idx].id == deviceId){
            switch (_devices[idx].type)
            {
                case Switch:
                {
                    auto device = esp32_switch_device(_devices[idx].pin);
                    device.setValue(value);
                    set = true;
                }
                    break;
                case Relay:
                {
                    auto device = esp32_relay_device(_devices[idx].pin);
                    device.setValue(value);
                    set = true;
                }
                    break;
            
                default:
                    break;
            }
            
            if(_devices[idx].mqttPublish && mqtt.enabled())
                mqtt.publish(_devices[idx].mqttTopic.c_str(), value  ? "On" : "Off");  
        }
    return set;
}
                 
                

vector<esp32_device_info> esp32_devices::_getDevices()
{
    StaticJsonDocument<2048> configFile;
    auto drive = filesystem.getDisk(0);
    File f = drive->open(PATH_DEVICE_CONFIG,"r");
    auto error = deserializeJson(configFile, f);
    if(error.code() != ESP_OK){
        Serial.printf("Failed to get devices. Received error deserializing configuration file. \n\t %d %s\n", error.code(), error.c_str());
    }
    f.close();    
    _devices.clear();
    if(configFile["devices"].isNull()){
        
        #ifdef DEBUG_DEVICE
        Serial.println("No devices found in configuration file");
        serializeJson(configFile, Serial);
        #endif

        return _devices;
    }
        
    auto devicesConfig = configFile["devices"].as<JsonArray>();    
    #ifdef DEBUG_DEVICE
    Serial.printf("Loaded device config:");
    serializeJson(devicesConfig, Serial);
    Serial.println();
    #endif

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
        if(!devicesConfig[idx]["resolution"].isNull()){
            deviceConfig.resolution = devicesConfig[idx]["resolution"].as<uint8_t>();
        } else deviceConfig.resolution = 10;
        
        deviceConfig.signal = (!devicesConfig[idx]["signal"].isNull() && strcmp(devicesConfig[idx]["signal"].as<const char*>(), "low") == 0) ? activeLow : activeHigh;

        deviceConfig.type = typeFromTypeName(devicesConfig[idx]["type"]);       
        if(deviceConfig.type == Relay)
            deviceConfig.duration =  devicesConfig[idx]["duration"].isNull() ? 5000 : devicesConfig[idx]["duration"].as<int>() * 1000;

        #ifdef DEBUG_DEVICE  
        Serial.printf("Adding device to list:\n\tID: \t\t%d\n\tName: \t\t%s\n\tType: \t\t%s%s\n\tPin: \t\t%d\n\tHas Trigger: \t%s\n\tSignal: \t%s\n",
            deviceConfig.id,    
            deviceConfig.name.c_str(),
            deviceConfig.type == AnalogInput ? "Analog" :
                deviceConfig.type == DigitalInput ? "Digital" :
                deviceConfig.type == Thermometer ? "Thermometer" :
                deviceConfig.type == Switch ? "Switch" : "Relay",
            deviceConfig.type == Relay ? string_format("\n\tDuration: 't%d seconds",deviceConfig.duration / 1000).c_str() : "",
            deviceConfig.pin,    
            deviceConfig.useTrigger ? "Yes" : "No" ,           
            deviceConfig.signal == activeHigh ? "Active High" : "Active Low" 
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
        if(deviceConfig.type == Thermometer){
            Serial.printf("\tResolution:\t%d\n",
                deviceConfig.resolution
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

vector<esp32_device_info> esp32_devices::getDevices()
{
    return _devices;
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
    #ifdef DEBUG_DEVICE
    Serial.printf("Setting snapshot frequency to %d seconds\n", _snapshotFrequency/1000);
    #endif
    
    return _getDevices().size() > 0;
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
