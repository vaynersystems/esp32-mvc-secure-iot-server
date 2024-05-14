#include "esp32_relay_device.hpp"

bool esp32_relay_device::getValue()
{
    return digitalRead(_pin);
}

void esp32_relay_device::setValue(bool value)
{
    digitalWrite(_pin, value);
    if(value) //record when relay was last turned on
        _lastOnTime = millis();
}

bool esp32_relay_device::turnOffIfTime()
{
    
    if(!getValue() || millis() - _lastOnTime < 5000) //TODO: configurable on time for relay
        return false;
    
    setValue(false);
    return true;
    
}
