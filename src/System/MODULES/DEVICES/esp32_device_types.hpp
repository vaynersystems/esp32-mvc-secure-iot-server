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

class esp32_device_info{
public:
    int id;
    esp32_device_type type;
    string name;
    int pin;
    esp32_device_direction direction;
    bool useTrigger = false;
    int triggerDeviceId = -1;
    esp32_device_trigger_type triggerType;
    double triggerValue;
    unsigned long duration;
    
    unsigned long _lastStartTime = 0;
    //esp32_base_device<uint16_t> deviceInstance;  

protected:
    
};

/* thiking out loud*/
struct esp32_output_device_next_state{
    int deviceId;
    bool state; //false for off, true for on
};
#endif

