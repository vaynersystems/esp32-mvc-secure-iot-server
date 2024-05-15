#include "esp32_relay_device.hpp"

bool esp32_relay_device::getValue()
{
    return digitalRead(_pin);
}

void esp32_relay_device::setValue(bool value)
{
    digitalWrite(_pin, value);
    if(value){ //record when relay was last turned on
        Serial.printf("Setting last on time for relay to: %lu\n",
            millis()
        );
        _lastOnTime = millis();
    }
}

bool esp32_relay_device::turnOffIfTime(unsigned long duration)
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
