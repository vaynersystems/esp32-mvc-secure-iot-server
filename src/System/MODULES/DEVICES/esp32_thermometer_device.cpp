#include "esp32_thermometer_device.hpp"

#include <DallasTemperature.h>
extern DallasTemperature sensors;

double esp32_thermometer_device::getValue()
{
    sensors.requestTemperaturesByIndex(0);
    double tempC = sensors.getTempFByIndex(0);
    return  tempC;
}

// void esp32_thermometer_device::setValue(uint16_t value)
// {
//     digitalWrite(_pin, value);
// }
