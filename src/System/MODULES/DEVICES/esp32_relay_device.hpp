#include "esp32_base_device.hpp"
class esp32_relay_device : public esp32_base_device<bool>{

public:
    esp32_relay_device(int pin, unsigned long lastOnMillis = 0) : esp32_base_device<bool>(pin){
        _lastOnTime = lastOnMillis;
    }
    
    bool getValue();

    void setValue(bool value);

    bool turnOffIfTime(unsigned long duration);

    esp32_device_type type(){ return esp32_device_type::Switch; }

protected:
    unsigned long _lastOnTime = 0;
};