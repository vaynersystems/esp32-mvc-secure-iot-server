#ifndef _ESP32_BASE_DEVICE_H
#define _ESP32_BASE_DEVICE_H
#include "Arduino.h"
#include "System/Config.h"
//#include "esp32_abstract_device.hpp"
enum esp32_device_type{
    Unknown = 0,
    Thermometer = 1,
    DigitalInput = 2,
    AnalogInput = 3,
    Switch = 4,
    Relay = 5
};

template<typename T>
class esp32_base_device{//} : public esp32_abstract_device{

public:
    esp32_base_device<T>(int pin){
        _pin = pin;
    }

    virtual ~esp32_base_device(){

    }
    
    virtual T getValue(){ return 0;}
    
    virtual void setValue(T value){};

    virtual esp32_device_type type(){ return esp32_device_type::Unknown;}

protected:

    int _pin;
    

};
#endif