#ifndef _ESP32_DEVICE_TYPES_H
#define _ESP32_DEVICE_TYPES_H
#include <string>
#include "esp32_base_device.hpp"

using namespace std;


enum esp32_device_direction{
    Input = 1,
    Output = 2
} ;

enum esp32_device_trigger_type{
    LessThan = 0,
    GreaterThan = 1,
    Equals = 2
};

enum esp32_device_signal{
    activeLow = 0,
    activeHigh = 1
};

class esp32_device_info{
public:
    int id;
    esp32_device_type type;
    string name;
    string mqttTopic;
    int pin;
    uint8_t resolution;
    esp32_device_signal signal = esp32_device_signal::activeHigh;
    //esp32_device_direction direction;
    bool useTrigger = false;
    bool mqttPublish = false;
    int triggerDeviceId = -1;
    int mqttFrequency = 1; //in minutes
    esp32_device_trigger_type triggerType;
    double triggerValue;
    unsigned long triggerThreshold;
    unsigned long duration;
    
    unsigned long lastStartTime = 0;
    unsigned long lastPublishTime = 0;
    //esp32_base_device<uint16_t> deviceInstance;  
    bool managedByScheduler = false;
    bool initialized = false;

protected:
    
};

/* thiking out loud*/
struct esp32_output_device_next_state{
    int deviceId;
    bool state; //false for off, true for on
};
#endif

