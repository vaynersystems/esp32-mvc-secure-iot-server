#include "esp32_digital_input_device.hpp"

bool esp32_digital_input_device::getValue()
{
    return digitalRead(_pin);
}

void esp32_digital_input_device::setValue(bool value)
{
    digitalWrite(_pin, value);
}
