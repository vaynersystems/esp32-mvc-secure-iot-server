#ifndef _ESP32_DEVICES_H
#define _ESP32_DEVICES_H

#include <vector>
#include <utility>
#include "esp32_device_types.hpp"
#include "esp32_analog_input_device.hpp"
#include "esp32_digital_input_device.hpp"
#include "esp32_thermometer_device.hpp"
#include "esp32_switch_device.hpp"
#include "esp32_relay_device.hpp"
#include <System/CORE/esp32_config.h>
#include "System/MODULES/LOGGING/esp32_logging.hpp"
#include "System/MODULES/MQTT/esp32_mqtt_client.hpp"
#include "System/MODULES/PINS/esp32_pin_manager.hpp"

#include "ArduinoJson.h"
#include <system_helper.h>
#include "string_helper.h"

#include <OneWire.h>
#include <DallasTemperature.h>

using namespace std;
extern DallasTemperature sensors;
extern esp32_logging logger;
extern esp32_mqtt_client mqtt;
extern esp32_pin_manager pinManager;
class esp32_devices{

public:
    /// @brief Load device configuration from SPIFFS volume
    esp32_devices(){};
    void onInit();

    void onLoop();

    void onDestroy();

    StaticJsonDocument<512>* getLastSnapshot();

    vector<esp32_device_info> getDevices();
    void getDeviceState(int deviceId, JsonObject object);
    bool setDeviceState(int deviceId, bool value);


    static esp32_device_type typeFromTypeName(const char * typeName);
    static esp32_device_trigger_type triggerTypeFromName(const char * triggerTypeName);

    static bool getDesiredState(
        bool currentState,
        esp32_device_trigger_type triggerType, 
        JsonVariant value, 
        double triggerValue, 
        unsigned long triggerThreshold
    );

protected:
    bool loadDeviceConfiguration();

    
private:
    vector<esp32_device_info> _getDevices();
    vector<esp32_device_info> _devices;
    unsigned long _lastSnapshotTime = 0, _lastSnapshotStoreTime = 0;

    unsigned long _snapshotFrequency = 60*1000;

    static JsonObject findDeviceState(JsonArray deviceStates, int deviceId);
    static int findDeviceStateIndex(JsonArray deviceStates, int deviceId);

    


    static bool isLessThan(bool currentState, JsonVariant value, double triggerValue, unsigned long triggerThreshold);
    static bool isGreaterThan(bool currentState, JsonVariant value, double triggerValue, unsigned long triggerThreshold);
    static bool isEqualTo(JsonVariant value, double triggerValue);

    /* For temperature sensors*/
    OneWire oneWire;
    
    StaticJsonDocument<512> _snapshot;
    StaticJsonDocument<512> _scratchpad;
    
};


#endif