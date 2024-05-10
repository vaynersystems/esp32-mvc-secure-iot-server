#include <vector>
#include <utility>
#include <System/CORE/esp32_config.h>
#include "ArduinoJson.h"
using namespace std;

enum esp32_device_type{
    Unknown = 0,
    Thermometer = 1,
    DigitalInput = 2,
    AnalogInput = 3,
    Switch = 4,
    Relay = 5
};

enum esp32_device_direction{
    Input = 1,
    Output = 2
} ;

enum esp32_device_trigger_type{
    LessThan = 0,
    GreaterThan = 1,
    Equals = 2
};

struct esp32_device_info{
    int id;
    esp32_device_type type;
    string name;
    int pin;
    esp32_device_direction direction;
    bool useTrigger = false;
    int triggerDeviceId = -1;
    esp32_device_trigger_type triggerType;
    double triggerValue;


};

/* thiking out loud*/
struct esp32_output_device_next_state{
    int deviceId;
    bool state; //false for off, true for on
};

class esp32_devices{

public:
    /// @brief Load device configuration from SPIFFS volume
    esp32_devices(){
        loadDeviceConfiguration();
    }

    vector<esp32_device_info> GetDevices();
    esp32_device_info GetDevice(int id);
    vector<pair<int,bool>> GetDeviceStates();
    template <typename T>
    T GetDeviceState(int id);
    template <typename T>
    bool SetDeviceState(int id, T state);

private:
    bool loadDeviceConfiguration();
    vector<esp32_device_info> _devices;
};