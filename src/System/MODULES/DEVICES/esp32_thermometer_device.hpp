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
        double tempC = sensors.getTempFByIndex(0);
        return  round(tempC * 10) / 10; //round to 1 significant digit
    }

    esp32_device_type type(){ return esp32_device_type::Thermometer; }
};