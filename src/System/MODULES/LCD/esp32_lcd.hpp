#ifndef ESP32_LCD_H
#define ESP32_LCD_H

#include "System/Config.h"
#include "LiquidCrystal_I2C.h"
#include <string>
#include <Wire.h>  // I2C library

using namespace std;
#define LCD_WIDTH 16

class esp32_lcd{
    public:

    void begin(int sda, int scl);
    void loop();

    void setTitle(const char * text);
    void setDetails(const char * text);
    void set(const char *title, const char * details = "");
    void setScroll(bool shouldScroll);
    void clear();

    // size_t write(uint8_t);
    // size_t print(const String &);
    // size_t print(const char*);
    // size_t print(char);
    // size_t print(unsigned char, int = DEC);
    // size_t print(int, int = DEC);
    // size_t print(unsigned int, int = DEC);
    // size_t print(long, int = DEC);
    // size_t print(unsigned long, int = DEC);
    // size_t print(long long, int = DEC);
    // size_t print(unsigned long long, int = DEC);
    // size_t print(double, int = 2);

    private:

    bool _scrollEnabled = true;
    string _title = "", _details = "";
    unsigned long _lastScrollTime = 0;
    LiquidCrystal_I2C _lcd;
    int _offset = 0;
    unsigned long _scrollSpeed = 500;
    int _spaceBetweenMessage = 3;
};
#endif