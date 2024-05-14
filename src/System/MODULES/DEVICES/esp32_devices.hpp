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

protected:
    bool loadDeviceConfiguration();
    
    //esp32_device_info GetDevice(int id);
    //vector<pair<int,bool>> GetDeviceStates();
    template <typename T>
    T getDeviceState(int id);
    template <typename T>
    bool setDeviceState(int id, T state);

    
private:
    vector<esp32_device_info> getDevices();
    vector<esp32_device_info> _devices;
    unsigned long _lastSnapshotTime = 0, _lastSnapshotStoreTime = 0;

    template<typename T>
    esp32_base_device<T>* getDevice(T type);
    void logSnapshot(JsonArray snapshot);

    static JsonObject findDeviceState(JsonArray devices, int deviceId);

    bool getDesiredState(
        bool currentState,
        esp32_device_trigger_type triggerType, 
        JsonVariant value, 
        double triggerValue, 
        const char * valueType
    );

    static esp32_device_type typeFromTypeName(const char * typeName);

    static bool isLessThan(JsonVariant value, double triggerValue, const char * type);
    static bool isGreaterThan(JsonVariant value, double triggerValue, const char * type);
    static bool isEqualTo(JsonVariant value, double triggerValue, const char * type);

    /* For temperature sensors*/
    OneWire oneWire;
    
    
    
};


