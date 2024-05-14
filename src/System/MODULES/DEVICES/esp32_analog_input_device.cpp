#include "esp32_analog_input_device.hpp"

uint16_t esp32_analog_input_device::getValue()
{
    return analogRead(_pin);
}

void esp32_analog_input_device::setValue(uint16_t value)
{
    analogWrite(_pin, value);
}
