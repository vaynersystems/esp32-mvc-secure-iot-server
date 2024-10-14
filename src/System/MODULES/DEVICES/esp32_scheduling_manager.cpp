#include "esp32_scheduling_manager.hpp"
#include "system_helper.h"

void esp32_scheduling_manager::onInit()
{
    #ifdef DEBUG_DEVICE
    Serial.println(F("Initializing schedule manager"));
    #endif

    if(!loadScheduleConfiguration()){
        Serial.println("Failed to initialize schedule configuration\n");
        logger.logError("Failed to initialize schedule configuration\n");
    }
}
StaticJsonDocument<4096> _scratch;

void esp32_scheduling_manager::onLoop()
{
    //get relevant schedules to now
    time_t now;
    now = getTime();
    struct tm tm = *localtime(&now);
    auto date = string_format("%02d/%02d/%d %02d:%02d:%02d", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
    auto devices = deviceManager.getDevices();
    for(auto device : devices){
        device.managedByScheduler = false;
    }

    for(int scheduleIdx = 0; scheduleIdx < _schedules.size(); scheduleIdx++){
        auto scheduleEntry = _schedules[scheduleIdx];
        //configured for day
        if(count(scheduleEntry.days.begin(), scheduleEntry.days.end(), tm.tm_wday) <= 0)
            continue;

        //after start time
        if(tm.tm_hour < scheduleEntry.startHour || (tm.tm_hour == scheduleEntry.startHour  && tm.tm_min < scheduleEntry.startMinute ))
            continue;
        
        //before end time
        if(tm.tm_hour > scheduleEntry.endHour || (tm.tm_hour == scheduleEntry.endHour  && tm.tm_min > scheduleEntry.endMinute ))
            continue;

        //active schedule entry, for each device in entry, check device status and set accordingly        
        for (int deviceIdx = 0; deviceIdx < devices.size(); deviceIdx++){ 
            if(devices[deviceIdx].id)
            if(count(scheduleEntry.deviceIds.begin(), scheduleEntry.deviceIds.end(), devices[deviceIdx].id) <= 0)
                continue; //device not present in current schedule

            //_scratch.clear(); causes memory leak

            #if DEBUG_SCHEDULE > 0
            Serial.printf("*** %02d:%02d\nSchedule %s. %02d:%02d - %02d:%02d.  Determining device [%d]%s state\n", 
                tm.tm_hour, tm.tm_min,
                scheduleEntry.name.c_str(), scheduleEntry.startHour,  scheduleEntry.startMinute,  scheduleEntry.endHour,  scheduleEntry.endMinute,
                devices[deviceIdx].id, devices[deviceIdx].name.c_str());
            #endif
            
            #if DEBUG_SCHEDULE > 1
            Serial.printf("schedule entry:\n\tID: \t\t%d\n\tName: \t\t%s\n\tStart: \t\t%02d:%02d\n\tEnd: \t\t%02d:%02d\n\tHas Trigger: \t%s\n",
                scheduleEntry.scheduleId,    
                scheduleEntry.name.c_str(),
                scheduleEntry.startHour, scheduleEntry.startMinute,
                scheduleEntry.endHour, scheduleEntry.endMinute,
                scheduleEntry.useTrigger ? "Yes" : "No" 
            );

            if(scheduleEntry.useTrigger){
                Serial.printf("\t   Source: \t%d\n\t   Type: \t%c\n\t   Value\t%lf\n\t   Threshold:\t%lu\n",
                    scheduleEntry.triggerDeviceId,
                    scheduleEntry.triggerType == LessThan ? '<' : 
                        scheduleEntry.triggerType == GreaterThan? '>' : '=',
                    scheduleEntry.triggerValue,
                    scheduleEntry.triggerThreshold

                );
            }
            #endif
            
            JsonObject sourceDeviceState = _scratch.to<JsonObject>();
            deviceManager.getDeviceState(scheduleEntry.triggerDeviceId, sourceDeviceState);
            
            bool electricCurrentState = digitalRead(devices[deviceIdx].pin);
            if(devices[deviceIdx].type != Switch && devices[deviceIdx].type != Relay)
            continue; //not an output device

            
            bool currentState = devices[deviceIdx].signal == activeLow ? !electricCurrentState : electricCurrentState;
            auto desiredState = esp32_devices::getDesiredState(currentState, scheduleEntry.triggerType, sourceDeviceState["value"], scheduleEntry.triggerValue, scheduleEntry.triggerThreshold);
            bool desiredElectricalState = devices[deviceIdx].signal == activeLow ? !desiredState : desiredState;
            
            
            if(!currentState && desiredState){
                logger.logInfo(string_format("Schedule %s: %s ON at %s\n", _schedules[scheduleIdx].name.c_str(), devices[deviceIdx].name.c_str(), date.c_str()).c_str(), esp32_log_type::device);
                #ifdef DEBUG_DEVICE
                Serial.printf(string_format("Schedule %s: %s ON at %s\n", _schedules[scheduleIdx].name.c_str(), devices[deviceIdx].name.c_str(), date.c_str()).c_str());
                #endif
            }
                
            else if(currentState && !desiredState){
                logger.logInfo(string_format("Schedule %s: %s OFF at %s\n", _schedules[scheduleIdx].name.c_str(), devices[deviceIdx].name.c_str(), date.c_str()).c_str(), esp32_log_type::device);
                #ifdef DEBUG_DEVICE
                Serial.printf(string_format("Schedule %s: %s OFF at %s\n", _schedules[scheduleIdx].name.c_str(), devices[deviceIdx].name.c_str(), date.c_str()).c_str());
                #endif
            }
                

            //if desired state is different that current state, change it
            //SET NEW VALUE TO DEVICE!!
            if(electricCurrentState != desiredElectricalState){
                //if managed by another schedule and turned off, do not do anything
                if(devices[deviceIdx].managedByScheduler && currentState)
                    continue;
                deviceManager.setDeviceState(devices[deviceIdx].id, desiredElectricalState);
            }

            if(devices[deviceIdx].type == Relay){
                auto deviceRelay = esp32_relay_device(devices[deviceIdx].pin, devices[deviceIdx].lastStartTime);
                if(deviceRelay.turnOffIfTime(devices[deviceIdx].duration))
                logger.logInfo(string_format("%s OFF at %s", devices[deviceIdx].name.c_str(), date.c_str()).c_str(), esp32_log_type::device);
            }
            //mark device as managed by scheduler for this frame
            devices[deviceIdx].managedByScheduler = true;
        }      
    }
}

bool esp32_scheduling_manager::loadScheduleConfiguration()
{
    
    StaticJsonDocument<2048> configFile;
    auto drive = filesystem.getDisk(0);
    File f = drive->open(PATH_DEVICE_CONFIG,"r");
    auto error = deserializeJson(configFile, f);
    if(error.code() != ESP_OK){
        Serial.printf("Failed to get schedules. Received error deserializing configuration file. \n\t %d %s\n", error.code(), error.c_str());
    }
    f.close();    
    _schedules.clear();
    if(configFile["schedules"].isNull()){
        
        #ifdef DEBUG_DEVICE
        Serial.println("No schedules found in configuration file");
        serializeJson(configFile, Serial);
        #endif

        return false;
    }
        
    auto schedulesConfig = configFile["schedules"].as<JsonArray>();    
    #if DEBUG_SCHEDULE > 0
    Serial.printf("Loaded schedules config:");
    serializeJson(schedulesConfig, Serial);
    Serial.println();
    #endif

    for(int idx = 0; idx < schedulesConfig.size();idx++){
        esp32_schedule scheduleItem;
        scheduleItem.scheduleId = schedulesConfig[idx]["id"];
        scheduleItem.name = schedulesConfig[idx]["name"].as<string>();
        if(!schedulesConfig[idx]["days"].isNull())
        {
            auto configDays = schedulesConfig[idx]["days"].as<JsonArray>();
            if(configDays.size() > 0)
                for(auto configDay : configDays)
                    scheduleItem.days.push_back(configDay.as<int>());
        }
        if(!schedulesConfig[idx]["devices"].isNull())
        {
            auto configDays = schedulesConfig[idx]["devices"].as<JsonArray>();
            if(configDays.size() > 0)
                for(auto configDay : configDays)
                    scheduleItem.deviceIds.push_back(configDay.as<int>());
        }
        
        
        if(!schedulesConfig[idx]["start"].isNull()){
            auto startTime = schedulesConfig[idx]["start"].as<string>();
            auto startParts = explode(startTime,":");    
            scheduleItem.startHour = atoi(startParts[0].c_str());
            scheduleItem.startMinute = atoi(startParts[1].c_str());
            if(scheduleItem.startHour < 0 || scheduleItem.startHour > 24)
                scheduleItem.startHour = 0;
            if(scheduleItem.startMinute < 0 || scheduleItem.startMinute > 59)
                scheduleItem.startMinute = 0;

        }
        if(!schedulesConfig[idx]["end"].isNull()){
            auto endTime = schedulesConfig[idx]["end"];
            auto endParts = explode(endTime,":");
            scheduleItem.endHour = atoi(endParts[0].c_str());
            scheduleItem.endMinute = atoi(endParts[1].c_str());
            if(scheduleItem.endHour < 0 || scheduleItem.endHour > 24)
                scheduleItem.endHour = 0;
            if(scheduleItem.endMinute < 0 || scheduleItem.endMinute > 59)
                scheduleItem.endMinute = 0;
        }
        
        if(
            !schedulesConfig[idx]["trigger"].isNull() && 
            !schedulesConfig[idx]["trigger"]["source"].isNull() && 
            !schedulesConfig[idx]["trigger"]["type"].isNull()
        ){
            scheduleItem.useTrigger = true;
            scheduleItem.triggerDeviceId = schedulesConfig[idx]["trigger"]["source"];
            scheduleItem.triggerType = esp32_devices::triggerTypeFromName(schedulesConfig[idx]["trigger"]["type"]);
            scheduleItem.triggerValue = schedulesConfig[idx]["trigger"]["value"];
            scheduleItem.triggerThreshold = schedulesConfig[idx]["trigger"]["threshold"];
        }
        
        #if DEBUG_SCHEDULE > 0
        Serial.printf("Adding schedule entry to list:\n\tID: \t\t%d\n\tName: \t\t%s\n\tStart: \t\t%02d:%02d\n\tEnd: \t\t%02d:%02d\n\tHas Trigger: \t%s\n",
            scheduleItem.scheduleId,    
            scheduleItem.name.c_str(),
            scheduleItem.startHour, scheduleItem.startMinute,
            scheduleItem.endHour, scheduleItem.endMinute,
            scheduleItem.useTrigger ? "Yes" : "No" 
        );

        if(scheduleItem.useTrigger){
            Serial.printf("\t   Source: %d\n\t   Type: \t%c\n\t   Value\t%lf\n\t   Threshold:\t%lu\n",
                scheduleItem.triggerDeviceId,
                scheduleItem.triggerType == LessThan ? '<' : 
                    scheduleItem.triggerType == GreaterThan? '>' : '=',
                scheduleItem.triggerValue,
                scheduleItem.triggerThreshold

            );
        }
        #endif

        _schedules.push_back(scheduleItem);
    }

    return true;
}

vector<esp32_schedule> esp32_scheduling_manager::getSchedules()
{
    return _schedules;
}