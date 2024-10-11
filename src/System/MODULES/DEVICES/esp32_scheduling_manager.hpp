#ifndef ESP32_SCHEDULING_MANAGER_H
#define ESP32_SCHEDULING_MANAGER_H
#include <vector>
#include <string>
#include <algorithm>
#include "System/MODULES/DEVICES/esp32_devices.hpp"
#include "string_helper.h"
using namespace std;
extern esp32_devices deviceManager;

struct esp32_schedule{
    short scheduleId;
    string name;
    vector<int> deviceIds;
    vector<int> days;
    int startHour;
    int startMinute;
    int endHour;
    int endMinute;
    bool useTrigger = false;    
    int triggerDeviceId = -1;
    esp32_device_trigger_type triggerType;
    double triggerValue;
    unsigned long triggerThreshold;
    unsigned long duration;
    
};

class esp32_scheduling_manager{
public:
    void onInit();
    void onLoop();

    vector<esp32_schedule> getSchedules();

private:
    bool getDesiredState(
        bool currentState,
        esp32_device_trigger_type triggerType, 
        JsonVariant value, 
        double triggerValue, 
        unsigned long triggerThreshold
    );
    bool loadScheduleConfiguration();
    vector<esp32_schedule> _schedules;
    //vector<esp32_device_info> devices = deviceManager.getDevices();
};
#endif