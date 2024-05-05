#include "esp32_devices.hpp"
#include <esp32-hal-gpio.h>

vector<esp32_device_info> esp32_devices::GetDevices()
{
    return vector<esp32_device_info>();
}

esp32_device_info esp32_devices::GetDevice(int id)
{
    return esp32_device_info();
}

vector<pair<int, bool>> esp32_devices::GetDeviceStates()
{
    return vector<pair<int, bool>>();
}
template <typename T>
T esp32_devices::GetDeviceState(int id)
{
    auto device = GetDevice(id);
    // according to esp32-hal-gpio, pin state can be read even when its in output mode
    // so no need to set state.
    
    auto analogChannel = digitalPinToAnalogChannel(device.pin);
    if(analogChannel >= 0)
        return analogRead(device.pin);
    
    return digitalRead(device.pin);
}

bool esp32_devices::SetDeviceState(int id, bool state)
{
    auto device = GetDevice(id);
    if(device.direction != esp32_device_direction::Output)
        return false;

    //set pin mode, write output
    pinMode(device.pin, OUTPUT);
    digitalWrite(device.pin, state);
    return true;
}
