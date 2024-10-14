#include "esp32_base_device.hpp"
#include <DallasTemperature.h>
extern DallasTemperature sensors;
class esp32_thermometer_device : public esp32_base_device<double>{

public:
    esp32_thermometer_device(int pin) : esp32_base_device<double>(pin){
    }  
    inline double getValue()
    {
        sensors.requestTemperatures();
        //sensors.requestTemperaturesByIndex(0);
        double tempF = sensors.getTempFByIndex(0);
        auto tempRounded = round(tempF * 100) / 100;
        #if DEBUG_DEVICE > 1
            Serial.printf("%s on pin %d reading: %f\n", "Thermometer", _pin, tempRounded);
        #endif
        return  round(tempF * 100) / 100; //round to 2 significant digit
    }

    esp32_device_type type(){ return esp32_device_type::Thermometer; }
};