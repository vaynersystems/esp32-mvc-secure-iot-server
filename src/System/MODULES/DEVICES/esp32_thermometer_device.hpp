#include "esp32_base_device.hpp"
class esp32_thermometer_device : public esp32_base_device<double>{

public:
    esp32_thermometer_device(int pin) : esp32_base_device<double>(pin){
    }
    
    double getValue();

    //void setValue(uint16_t value);

    esp32_device_type type(){ return esp32_device_type::Thermometer; }
};