#include "esp32_devices.hpp"
#include <esp32-hal-gpio.h>


vector<esp32_device_info> esp32_devices::GetDevices()
{
    vector<esp32_device_info> devices;
    StaticJsonDocument<1024> devicesConfig;
    esp32_config::getConfigSection("devices", &devicesConfig);

    for(int idx = 0; idx < devicesConfig.size();idx++){
        esp32_device_info deviceConfig;
        deviceConfig.direction = devicesConfig[idx]["direction"];
        deviceConfig.id = devicesConfig[idx]["direction"];
        deviceConfig.name = devicesConfig[idx]["name"].as<string>();
        deviceConfig.pin = devicesConfig[idx]["pin"];
        deviceConfig.triggerDeviceId = devicesConfig[idx]["triggerDeviceId"];
        deviceConfig.triggerType = devicesConfig[idx]["triggerType"];
        deviceConfig.triggerValue = devicesConfig[idx]["triggerValue"];
        deviceConfig.type = devicesConfig[idx]["type"];
        deviceConfig.useTrigger = devicesConfig[idx]["useTrigger"];
        devices.push_back(deviceConfig);
    }

    return devices;
}

esp32_device_info esp32_devices::GetDevice(int id)
{
    return esp32_device_info();
}

vector<pair<int, bool>> esp32_devices::GetDeviceStates()
{
    vector<pair<int, bool>> deviceStates;
    auto foundDevices = GetDevices();
    for(int idx=0; idx< foundDevices.size(); idx++){
        deviceStates.push_back(pair<int, bool>(foundDevices[idx].id, GetDeviceState<bool>(foundDevices[idx].id)));
    }
    return deviceStates;
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

template <typename T>
bool esp32_devices::SetDeviceState(int id, T state)
{
    auto device = GetDevice(id);
    if(device.direction != esp32_device_direction::Output)
        return false;

    //set pin mode, write output
    pinMode(device.pin, OUTPUT);
    digitalWrite(device.pin, state);
    return true;
}
