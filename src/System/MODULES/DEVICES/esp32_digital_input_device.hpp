#include "esp32_base_device.hpp"
class esp32_digital_input_device : public esp32_base_device<bool>{
public:
    esp32_digital_input_device(int pin) : esp32_base_device<bool>(pin){
    }
    inline  bool getValue()
    {
        return digitalRead(_pin);
    }

    inline void setValue(bool value)
    {
        digitalWrite(_pin, value);
    }

    inline esp32_device_type type(){ return esp32_device_type::DigitalInput; }

};