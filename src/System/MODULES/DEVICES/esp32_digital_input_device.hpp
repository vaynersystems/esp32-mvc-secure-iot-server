#include "esp32_base_device.hpp"
class esp32_digital_input_device : public esp32_base_device<bool>{
public:
    esp32_digital_input_device(int pin) : esp32_base_device<bool>(pin){
    }
    bool getValue();

    void setValue(bool value);

    esp32_device_type type(){ return esp32_device_type::DigitalInput; }

};