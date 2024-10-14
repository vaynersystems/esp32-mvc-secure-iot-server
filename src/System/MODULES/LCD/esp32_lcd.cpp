#include "esp32_lcd.hpp"
#include <WiFi.h>

#include "System/MODULES/DEVICES/esp32_devices.hpp"
extern esp32_devices deviceManager;

void esp32_lcd::begin(int sda, int scl)
{
    _lcd.begin(LCD_WIDTH,2,LCD_5x8DOTS, sda, scl);
    _lcd.clear();        
    //Serial.println("Initialized LCD");
    _messages.push_back(esp32_lcd_message("IP Address:","IP"));
    _messages.push_back(esp32_lcd_message("Time:", "TIME"));    
}

void esp32_lcd::loop()
{
    //if in text mode, timeout to message mode after [configured time] of inactivitiy
    if(_mode != elm_messages && _lastTextTime + _textTimeout < millis()){
        _mode = elm_messages;
    }


    if(_mode == elm_messages){
        //go to next message if time
        if(_lastMessageTime + _messageTimeout < millis()){
            //Serial.printf("Printing the %d%s message\n", _messageIdx + 1, _messageIdx + 1 == 1 ? "st" : _messageIdx + 1 == 2 ? "nd": _messageIdx + 1 == 3 ? "rd" : "th" );
            auto parts = explode(string(_messages[_messageIdx].messageText), ":", true);
            clear();
            setTitle( parts[0].c_str(), elm_messages);
            Serial.printf("Checking parameter value [%s]\n",_messages[_messageIdx].messageParam.c_str());
            if (strcmp(_messages[_messageIdx].messageParam.c_str(), "IP") == 0)
            {
                setDetails(WiFi.localIP().toString().c_str(), elm_messages);                
            } 
            else if(strcmp(_messages[_messageIdx].messageParam.c_str(), "TIME") == 0){  
                setDetails(getCurrentTime().c_str(), elm_messages);
            }
            else {
                int paramLength = _messages[_messageIdx].messageParam.length();
                if(paramLength >= 4){                   
                    
                    if(strcmp(_messages[_messageIdx].messageParam.substr(0,3).c_str(), "DEV") == 0){  
                        int deviceId = parseInt(_messages[_messageIdx].messageParam.substr(3));
                        
                        auto snapshotFile = deviceManager.getLastSnapshot();
                        JsonArray devicesInSnapshot = (*snapshotFile)["series"].as<JsonArray>();

                        for(int deviceIdx = 0; deviceIdx < devicesInSnapshot.size(); deviceIdx++){
                            //auto device = deviceManager.getDevices().at(deviceIdx)
                            if(devicesInSnapshot[deviceIdx]["id"].isNull()) continue;
                            if(devicesInSnapshot[deviceIdx]["id"].as<int>() == deviceId)  
                            {
                                if(devicesInSnapshot[deviceIdx]["value"].isNull()){
                                    Serial.printf("Value is null!!\n");
                                    break;
                                }
                                if(devicesInSnapshot[deviceIdx]["value"].is<uint16_t>()){
                                    uint16_t sourceDeviceValue = devicesInSnapshot[deviceIdx]["value"].as<uint16_t>();
                                    setDetails(string_format("%u", sourceDeviceValue).c_str(), elm_messages);

                                } 
                                else if(devicesInSnapshot[deviceIdx]["value"].is<double>()){
                                    double sourceDeviceValue = devicesInSnapshot[deviceIdx]["value"].as<double>();
                                    setDetails(string_format("%02.02f", sourceDeviceValue).c_str(), elm_messages);
                                }
                                else if(devicesInSnapshot[deviceIdx]["value"].is<float>()){
                                    float sourceDeviceValue = devicesInSnapshot[deviceIdx]["value"].as<float>();
                                    setDetails(string_format("%02.02f", sourceDeviceValue).c_str(), elm_messages);

                                }
                                else if(devicesInSnapshot[deviceIdx]["value"].is<bool>()){    
                                    bool value = devicesInSnapshot[deviceIdx]["value"].as<bool>();
                                    setDetails(value ? "ON" : "OFF", elm_messages);
                                }
                                else if(devicesInSnapshot[deviceIdx]["value"].is<int>()){
                                    int sourceDeviceValue = devicesInSnapshot[deviceIdx]["value"].as<int>();
                                    setDetails(itoa(sourceDeviceValue,"%d",10), elm_messages);
                                }
                                else if(devicesInSnapshot[deviceIdx]["value"].is<short>()){
                                    double sourceDeviceValue = devicesInSnapshot[deviceIdx]["value"].as<short>();
                                    setDetails(itoa(sourceDeviceValue,"%d",10), elm_messages);
                                }
                                else if(devicesInSnapshot[deviceIdx]["value"].is<const char *>()) {
                                    Serial.printf("Error occured checking less than condition. value type is const char * %d\n", devicesInSnapshot[deviceIdx]["value"].as<const char *>());
                                    continue;
                                }
                                else {
                                    Serial.printf("Error occured checking less than condition. value type is unknown %d : %s\n", devicesInSnapshot[deviceIdx]["value"].as<int>(),  devicesInSnapshot[deviceIdx]["value"].as<const char *>());
                                    continue;
                                }

                                                               
                            }
                        }
                        
                        
                    }
                }

               
            }

            _messageIdx ++;
            if(_messageIdx >= _messages.size()) _messageIdx = 0;

            _lastMessageTime = millis();
        }
    }
    //time to scroll?
    //Serial.printf("Checking if time %lu is time to scroll at %lu\n", _lastScrollTime + _scrollSpeed, millis());
    if(_lastScrollTime + _scrollSpeed > millis())
        return;

    //Serial.printf("Checking if there is content to scroll with length %d\n", _details.length());
    //anything to scroll?
    if(_details.length() <= LCD_WIDTH)
        return;

    _lcd.setCursor(0,1);

    //Serial.printf("Checking if offset %d needs to be reset: %s\n", _offset, _offset >= LCD_WIDTH ? "Yes" : "No");
    if(_offset >= LCD_WIDTH) _offset = 0;
    else _offset++;

    int charsLeft = _details.length() - _offset;

    if(charsLeft <= LCD_WIDTH){        
        //can fit rest of string
        _lcd.setCursor(0,1);
        _lcd.print(_details.substr(_offset,charsLeft).c_str());
        int charsWritten = charsLeft - _offset;
        //Serial.printf("Wrote %d chars to lcd details (all)\n", charsWritten);
        
        if(LCD_WIDTH - charsWritten > _spaceBetweenText){
            //enough room to wrap around
            _lcd.print(string(_spaceBetweenText, ' ').c_str());
            int screenOffset = charsWritten + _spaceBetweenText;
            //_lcd.setCursor(screenOffset,1);
            //Serial.printf("Writing %d additional chars from beggining at position %d\n", LCD_WIDTH - screenOffset, screenOffset);
            _lcd.print(_details.substr(0, LCD_WIDTH - screenOffset).c_str());
        }
    }else{
        //only enough room to print some of the text
        _lcd.print(_details.substr(_offset, LCD_WIDTH).c_str());
        //Serial.printf("Wrote %d chars to lcd details (some) \n", LCD_WIDTH);
    }
    _lastScrollTime = millis();
    //Serial.printf("Updating last scroll time to %lu\n", _lastScrollTime);
    
}

void esp32_lcd::setTitle(const char *text, esp32_lcd_mode mode)
{
    _mode = mode;
    _title = text;
    _lcd.setCursor(0,0);
    _lcd.print(text);
}

void esp32_lcd::setDetails(const char *text, esp32_lcd_mode mode)
{
    _mode = mode;
    _details = text;
    _offset = 0;
    _lcd.setCursor(0,1);    
    _lcd.print(text);
    _lastScrollTime = millis();
}

void esp32_lcd::set(const char *title, const char *details, esp32_lcd_mode mode)
{
    _lcd.clear();
    setTitle(title, mode);
    setDetails(details, mode);
}

void esp32_lcd::clear()
{
    _lcd.clear();
}

void esp32_lcd::addMessage(const char *message, const char *parameter)
{
    //TODO: validate message and parameter
    _messages.push_back(esp32_lcd_message(message, parameter));    
}

// size_t esp32_lcd::write(uint8_t byte)
// {
//     _lcd.write(byte);
// }

// size_t esp32_lcd::print(const String & string)
// {
//      _lcd.print(string);
// }


// size_t esp32_lcd::print(const char * text)
// {
//     _lcd.print(text);
// }


// size_t esp32_lcd::print(char c)
// {
//     _lcd.print(c);
// }

// size_t esp32_lcd::print(unsigned char text, int length)
// {
//     _lcd.print(text, length);
// }

// size_t esp32_lcd::print(int value, int precision)
// {
//     _lcd.print(value, precision);
// }

// size_t esp32_lcd::print(unsigned int value, int precision)
// {
//     _lcd.print(value, precision);
// }

// size_t esp32_lcd::print(long value, int precision)
// {
//     _lcd.print(value, precision);
// }

// size_t esp32_lcd::print(unsigned long value, int precision)
// {
//     _lcd.print(value, precision);
// }

// size_t esp32_lcd::print(long long value, int precision)
// {
//     _lcd.print(value, precision);
// }

// size_t esp32_lcd::print(unsigned long long value, int precision)
// {
//     _lcd.print(value, precision);
// }

// size_t esp32_lcd::print(double value, int precision)
// {
//     _lcd.print(value, precision);
// }
esp32_lcd lcd;