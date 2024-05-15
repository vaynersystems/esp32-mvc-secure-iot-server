#include <vector>
#include <utility>
#include "esp32_device_types.hpp"
#include "esp32_analog_input_device.hpp"
#include "esp32_digital_input_device.hpp"
#include "esp32_thermometer_device.hpp"
#include "esp32_switch_device.hpp"
#include "esp32_relay_device.hpp"
#include <System/CORE/esp32_config.h>
#include "ArduinoJson.h"

#include <OneWire.h>
#include <DallasTemperature.h>

using namespace std;
extern DallasTemperature sensors;


class esp32_devices{

public:
    /// @brief Load device configuration from SPIFFS volume
    esp32_devices(){};
    void onInit();

    void onLoop();

    void onDestroy();

    StaticJsonDocument<2048>* getLastSnapshot();
    //vector<esp32_device_info> getDeviceConfiguration();

protected:
    bool loadDeviceConfiguration();
    
    //esp32_device_info GetDevice(int id);
    //vector<pair<int,bool>> GetDeviceStates();
    // template <typename T>
    // T getDeviceState(int id);
    // template <typename T>
    // bool setDeviceState(int id, T state);

    
private:
    vector<esp32_device_info> getDevices();
    vector<esp32_device_info> _devices;
    unsigned long _lastSnapshotTime = 0, _lastSnapshotStoreTime = 0;

    unsigned long _snapshotFrequency = 60*1000;
    int _retentionDays = 365;

    void removeOldLogs();

    // template<typename T>
    // esp32_base_device<T>* getDevice(T type);
    void logSnapshot(JsonObject snapshot);

    static JsonObject findDeviceState(JsonArray deviceStates, int deviceId);
    static int findDeviceStateIndex(JsonArray deviceStates, int deviceId);

    bool getDesiredState(
        bool currentState,
        esp32_device_trigger_type triggerType, 
        JsonVariant value, 
        double triggerValue, 
        unsigned long triggerThreshold
    );

    static esp32_device_type typeFromTypeName(const char * typeName);
    static esp32_device_trigger_type triggerTypeFromName(const char * triggerTypeName);

    static bool isLessThan(bool currentState, JsonVariant value, double triggerValue, unsigned long triggerThreshold);
    static bool isGreaterThan(bool currentState, JsonVariant value, double triggerValue, unsigned long triggerThreshold);
    static bool isEqualTo(JsonVariant value, double triggerValue);

    /* For temperature sensors*/
    OneWire oneWire;
    
    StaticJsonDocument<2048> _snapshot;
    
};


