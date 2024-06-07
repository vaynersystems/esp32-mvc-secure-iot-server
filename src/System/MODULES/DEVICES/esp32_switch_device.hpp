#include "esp32_base_device.hpp"
class esp32_switch_device : public esp32_base_device<bool>{

public:
    esp32_switch_device(int pin) : esp32_base_device<bool>(pin){
    }
    
    inline bool getValue()
    {
        return digitalRead(_pin);
    }

    inline void setValue(bool value)
    {
        digitalWrite(_pin, value);
    }


    esp32_device_type type(){ return esp32_device_type::Switch; }
};