#ifndef _ESP32_PIN_MANAGER_H
#define _ESP32_PIN_MANAGER_H
using namespace std;
#include <vector>
#include <Esp.h>
struct esp32_pin{
    short gpioPin;
    bool isAnalog;
    gpio_mode_t pinMode;
    esp32_pin(short gpioPin, bool isAnalog = true, gpio_mode_t pinMode = GPIO_MODE_INPUT_OUTPUT){
        this->gpioPin = gpioPin;
        this->isAnalog = isAnalog;
        this->pinMode = pinMode;
    }
};
static const esp32_pin _esp32_s3_pins[] = {
        esp32_pin(1),
        esp32_pin(2),
        esp32_pin(5),
        esp32_pin(6),
        esp32_pin(7),
        esp32_pin(8), // i2c scl
        esp32_pin(9), // i2c sda
        esp32_pin(10),// spi0_cs0
        esp32_pin(11),// spi0_mosi
        esp32_pin(12),// spi0_sck
        esp32_pin(13),// spi0_miso
        esp32_pin(14),
        esp32_pin(15),
        esp32_pin(16),
        esp32_pin(17),
        esp32_pin(18),
        esp32_pin(21, false),
        #if SD_TYPE != sd_mmc
        esp32_pin(38, false),
        esp32_pin(39, false),
        esp32_pin(40, false),
        esp32_pin(41, false),
        esp32_pin(42, false),
        esp32_pin(47),
        #endif
        esp32_pin(45),
        esp32_pin(48),
        
};

static const esp32_pin _esp32_d0wdq6_pins[] = {
        esp32_pin(4, true),
        esp32_pin(5, false), //outputs PWM at boot, strapping pin, spi cs
        esp32_pin(13), //v-spi  mosi
        esp32_pin(14), //outputs PWM at boot, v-spi miso
        esp32_pin(15), //outputs PWM at boot, strapping pin, v-spi cs
        esp32_pin(16, false), 
        esp32_pin(17, false), 
        esp32_pin(18, false), //spi clk
        esp32_pin(19, false), //spi miso
        esp32_pin(20, false),
        #ifndef USE_LCD
        esp32_pin(21, false), //i2c sda
        esp32_pin(22, false), //i2c scl
        #endif
        esp32_pin(23, false), //spi mosi
        esp32_pin(25),
        esp32_pin(26), //listed input only? but output verified!
        esp32_pin(27),
        esp32_pin(32),
        esp32_pin(33),
        esp32_pin(34, GPIO_MODE_INPUT),
        esp32_pin(35, GPIO_MODE_INPUT),
        esp32_pin(36, GPIO_MODE_INPUT),
        esp32_pin(39, GPIO_MODE_INPUT),
};

class esp32_pin_manager{
    public:
    vector<esp32_pin> getControllerPins(){
        if(_devicePins == nullptr){
            auto mcuName = ESP.getChipModel();
            if(mcuName == "ESP32-S3"){
                 _devicePins = new vector<esp32_pin>(_esp32_s3_pins, _esp32_s3_pins + sizeof(_esp32_s3_pins) / (sizeof(_esp32_s3_pins[0])));
            } else { //default
                _devicePins = new vector<esp32_pin>(_esp32_d0wdq6_pins, _esp32_d0wdq6_pins + sizeof(_esp32_d0wdq6_pins) / (sizeof(_esp32_d0wdq6_pins[0])));
            }
        }

        return *_devicePins;
    }

    private:
    vector<esp32_pin>* _devicePins = nullptr;

    
};
#endif