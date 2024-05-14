#include "esp32_switch_device.hpp"

bool esp32_switch_device::getValue()
{
    return digitalRead(_pin);
}

void esp32_switch_device::setValue(bool value)
{
    digitalWrite(_pin, value);
}
