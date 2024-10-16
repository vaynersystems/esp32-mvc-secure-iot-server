#include "esp32_base_device.hpp"
class esp32_relay_device : public esp32_base_device<bool>{

public:
    esp32_relay_device(int pin, unsigned long lastOnMillis = 0) : esp32_base_device<bool>(pin){
        pinMode(pin,OUTPUT);
        _lastOnTime = lastOnMillis;
    }
    
    inline bool getValue()
    {
        //pinMode(_pin,INPUT);
        auto result = digitalRead(_pin);
        //pinMode(_pin,OUTPUT);
         #if DEBUG_DEVICE > 1
        Serial.printf("Reading relay on pin %d: %s\n", _pin,  result ? "HIGH" : "LOW");
        #endif
        return result;
    }

    inline void setValue(bool value)
    {
         #if DEBUG_DEVICE > 1
            Serial.printf("Setting %s on pin %d reading: %d\n", "Relay", _pin, value);
        #endif
        digitalWrite(_pin, value);
        if(value){ //record when relay was last turned on
            Serial.printf("Setting last on time for relay to: %lu\n",
                millis()
            );
            _lastOnTime = millis();
        }
    }

    inline bool turnOffIfTime(unsigned long duration)
    {
        if(!getValue()) return false;
        if(millis() - _lastOnTime < duration)
            return false;
        
        Serial.printf("Detected that it is time to turn off relay. Current millis: %lu, start %lu\n",
            millis(), _lastOnTime
        );
        setValue(false);
        return true;
        
    }


    inline esp32_device_type type(){ return esp32_device_type::Switch; }

protected:
    unsigned long _lastOnTime = 0;
};