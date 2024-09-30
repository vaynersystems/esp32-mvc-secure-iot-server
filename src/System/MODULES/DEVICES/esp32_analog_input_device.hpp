#include "esp32_base_device.hpp"
class esp32_analog_input_device : public esp32_base_device<uint16_t>{

public:
    esp32_analog_input_device(int pin) : esp32_base_device<uint16_t>(pin){
        pinMode(_pin,INPUT);
    }

    ~esp32_analog_input_device(){
    }
    
    inline uint16_t getValue()
    {
        uint16_t value = analogRead(_pin);
        #ifdef DEBUG_DEVICE
            Serial.printf("%s on pin %d reading: %f\n", "Analog Input", _pin, value);
        #endif
        return value;
    }

    inline void setValue(uint16_t value)
    {
        #ifdef DEBUG_DEVICE
            Serial.printf("Setting %s on pin %d reading: %f\n", "Analog Input", _pin, value);
        #endif
        analogWrite(_pin, value);
    }


    inline esp32_device_type type(){ return esp32_device_type::AnalogInput; }
};