#include "esp32_base_device.hpp"
class esp32_digital_input_device : public esp32_base_device<bool>{
public:
    esp32_digital_input_device(int pin) : esp32_base_device<bool>(pin){
        pinMode(_pin,INPUT);
    }
    inline  bool getValue()
    {
        bool value = digitalRead(_pin);
        #ifdef DEBUG_DEVICE
            Serial.printf("%s on pin %d reading: %d\n", "Digital Input", _pin, value);
        #endif
        return value;
    }

    inline void setValue(bool value)
    {
        #ifdef DEBUG_DEVICE
            Serial.printf("Setting %s on pin %d reading: %d\n", "Digital Input", _pin, value);
        #endif
        digitalWrite(_pin, value);
    }

    inline esp32_device_type type(){ return esp32_device_type::DigitalInput; }

};