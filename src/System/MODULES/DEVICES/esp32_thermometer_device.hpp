#include "esp32_base_device.hpp"
#include <DallasTemperature.h>
#include <map>
extern DallasTemperature sensors;
class esp32_thermometer_device : public esp32_base_device<double>{

public:
    esp32_thermometer_device(int pin) : esp32_base_device<double>(pin){
    }  
    // Non-blocking: requestTemperatures() normally waits the whole ~750 ms conversion, and this
    // runs every device pass while the shared task lock is held, which starved the web server.
    // Start a conversion, return the last reading, and collect the result once it is done.
    inline double getValue()
    {
        // A new device object is made for every read, so the reading state lives per pin.
        ThermometerState& st = state();
        double& _lastValue = st.lastValue;
        bool& _haveReading = st.haveReading;
        bool& _conversionPending = st.conversionPending;
        unsigned long& _conversionStarted = st.conversionStarted;
        if(!_haveReading){
            // First reading only: wait once, so triggers never see an empty value.
            sensors.setWaitForConversion(true);
            sensors.requestTemperatures();
            _lastValue = round(sensors.getTempFByIndex(0) * 100) / 100;
            _haveReading = true;
            startConversion(st);
        } else if(_conversionPending &&
                  (sensors.isConversionComplete() ||
                   millis() - _conversionStarted >= sensors.millisToWaitForConversion(sensors.getResolution()))){
            _lastValue = round(sensors.getTempFByIndex(0) * 100) / 100; //round to 2 significant digit
            _conversionPending = false;
            startConversion(st);
        } else if(!_conversionPending){
            startConversion(st);
        }
        #if DEBUG_DEVICE > 1
            Serial.printf("%s on pin %d reading: %f\n", "Thermometer", _pin, _lastValue);
        #endif
        return _lastValue;
    }

    esp32_device_type type(){ return esp32_device_type::Thermometer; }

private:
    struct ThermometerState {
        double lastValue = 0;
        bool haveReading = false;
        bool conversionPending = false;
        unsigned long conversionStarted = 0;
    };
    ThermometerState& state(){
        static std::map<int, ThermometerState> states;
        return states[_pin];
    }
    static void startConversion(ThermometerState& st){
        sensors.setWaitForConversion(false);
        sensors.requestTemperatures(); // returns immediately
        st.conversionPending = true;
        st.conversionStarted = millis();
    }
};