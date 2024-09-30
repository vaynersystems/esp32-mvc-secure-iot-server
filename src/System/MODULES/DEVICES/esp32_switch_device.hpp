#include "esp32_base_device.hpp"
class esp32_switch_device : public esp32_base_device<bool>{

public:
    esp32_switch_device(int pin) : esp32_base_device<bool>(pin){
        pinMode(pin,OUTPUT);
    }
    
    inline bool getValue()
    {
        //pinMode(_pin,INPUT);
        bool result = digitalRead(_pin);
        //pinMode(_pin,OUTPUT);
        #ifdef DEBUG_DEVICE
        Serial.printf("Reading switch on pin %d: %s\n", _pin,  result == true ? "HIGH" : "LOW");
        #endif
        return result;
    }

    inline void setValue(bool value)
    {
        digitalWrite(_pin, value);
    }


    esp32_device_type type(){ return esp32_device_type::Switch; }
};