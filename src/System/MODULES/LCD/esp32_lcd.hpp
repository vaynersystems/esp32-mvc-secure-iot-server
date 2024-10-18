#ifndef ESP32_LCD_H
#define ESP32_LCD_H

#include "System/Config.h"
#include "ArduinoJson.h"

#include "LiquidCrystal_I2C.h"
#include "string_helper.h"
#include "system_helper.h"
#include <string>
#include <vector>
#include <Wire.h>  // I2C library



using namespace std;
#define LCD_WIDTH 16

enum esp32_lcd_mode{
    elm_text = 1,
    elm_messages = 2,
    elm_menu = 3
};

struct esp32_lcd_message{
    string messageText;
    string messageParam;
    esp32_lcd_message(const char * message, const char* param){
        messageText = message;
        messageParam = param;
    }
    esp32_lcd_message(string message, string param){
        messageText = message;
        messageParam = param;
    }
};

class esp32_lcd{
    public:

    void begin(int sda, int scl);
    void loop();

    void setTitle(const char * text, esp32_lcd_mode mode = elm_text, bool clearLine = true);
    void setDetails(const char * text, esp32_lcd_mode mode = elm_text, bool clearLine = true);
    void set(const char *title, const char * details = "", esp32_lcd_mode mode = elm_text);
    void setScroll(bool shouldScroll);
    void clear();

    void pause(){
        _paused = true;
    }
    void play(){
        _paused = false;
    }

    void addMessage(const char* message, const char* parameter);

    

    private:

    bool _scrollEnabled = true;
    bool _paused = false;
    bool _initialized = false;
    bool _updatePending = false;
    char _title[64] = {0}, _details[64] = {0};
    unsigned long _lastScrollTime = 0, _lastTextTime = 0, _lastMessageTime = 0;
    LiquidCrystal_I2C _lcd;
    int _offset = 0;
    unsigned long _scrollSpeed = 350, _textTimeout = 8000, _messageTimeout = 7000;
    int _spaceBetweenText = 3;
    esp32_lcd_mode _mode;
    vector<esp32_lcd_message> _messages;
    int _messageIdx = 0;


};
#endif