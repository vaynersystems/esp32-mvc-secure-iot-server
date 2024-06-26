#include "esp32_base_device.hpp"
class esp32_analog_input_device : public esp32_base_device<uint16_t>{

public:
    esp32_analog_input_device(int pin) : esp32_base_device<uint16_t>(pin){
    }

    ~esp32_analog_input_device(){
    }
    
    inline uint16_t getValue()
    {
        return analogRead(_pin);
    }

    inline void setValue(uint16_t value)
    {
        analogWrite(_pin, value);
    }


    inline esp32_device_type type(){ return esp32_device_type::AnalogInput; }
};