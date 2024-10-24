#include "esp32_lcd.hpp"
#include <WiFi.h>

#include "System/MODULES/DEVICES/esp32_devices.hpp"
#include "System/MODULES/SCHEDULING/esp32_scheduling_manager.hpp"

extern esp32_scheduling_manager scheduleManager;
extern esp32_devices deviceManager;

void esp32_lcd::begin(int sda, int scl)
{
    _lcd.begin(LCD_WIDTH,2,LCD_5x8DOTS, sda, scl);
    clear();        
    //Serial.println("Initialized LCD");
    _messages.push_back(esp32_lcd_message("IP Address:","IP"));
    _messages.push_back(esp32_lcd_message("Time:", "TIME"));   
    _messages.push_back(esp32_lcd_message("Uptime:", "UPTIME"));   
    _initialized = true; 
}

void esp32_lcd::loop()
{
    if(!_initialized) return;

    if(_updatePending){
        clear();
        
        _lcd.setCursor(0,0);         
        _lcd.print(_title);
        _lcd.setCursor(0,1);         
        _lcd.print(_details);
        
        _offset = 0;
        _lastScrollTime = millis();
        _lastMessageTime = millis();
        _updatePending = false;
        return;    
    }
    //if in text mode, timeout to message mode after [configured time] of inactivitiy
    if(_mode != elm_messages && _lastTextTime + _textTimeout < millis() && !_paused){
        _mode = elm_messages;
    }

    if(_mode == elm_messages){
        //go to next message if time
        if(_lastMessageTime + _messageTimeout < millis()){
            //Serial.printf("Printing the %d%s message\n", _messageIdx + 1, _messageIdx + 1 == 1 ? "st" : _messageIdx + 1 == 2 ? "nd": _messageIdx + 1 == 3 ? "rd" : "th" );
            auto parts = explode(string(_messages[_messageIdx].messageText), ":", true);
            
            setTitle( parts[0].c_str(), elm_messages);
            if (strcmp(_messages[_messageIdx].messageParam.c_str(), "IP") == 0)
            {
                if(WiFi.getMode() == WiFiMode_t::WIFI_MODE_STA)
                    setDetails(WiFi.localIP().toString().c_str(), elm_messages);                
                else if(WiFi.getMode() == WiFiMode_t::WIFI_MODE_AP)
                    setDetails(WiFi.softAPIP().toString().c_str(), elm_messages);                
            } 
            else if(strcmp(_messages[_messageIdx].messageParam.c_str(), "TIME") == 0){  
                setDetails(getCurrentTime().c_str(), elm_messages);
            }
            else if(strcmp(_messages[_messageIdx].messageParam.c_str(), "UPTIME") == 0){  
                int uptimeS = (int)(millis() / 1000);
                int uptimeSeconds = uptimeS % (60);
                int uptimeMinutes = ((uptimeS - uptimeSeconds) % 3600) / 60;
                int uptimeHours = ((uptimeS - uptimeSeconds - (uptimeMinutes * 60)) % (3600*24)) / 3600;
                int uptimeDays = (uptimeS - uptimeSeconds - (uptimeMinutes * 60) - (uptimeHours * 3600)) / (3600*24);
                setDetails(string_format("%03d - %02d:%02d:%02d", uptimeDays, uptimeHours, uptimeMinutes, uptimeSeconds).c_str(), elm_messages);
            }
            else {
                int paramLength = _messages[_messageIdx].messageParam.length();
                if(paramLength >= 4){                   
                    
                    if(strcmp(_messages[_messageIdx].messageParam.substr(0,5).c_str(), "DEV #") == 0){  
                        int deviceId = parseInt(_messages[_messageIdx].messageParam.substr(5));
                        //TODO: move to device manager
                        auto snapshotFile = deviceManager.getLastSnapshot();
                        JsonArray devicesInSnapshot = (snapshotFile)["series"].as<JsonArray>();
                        char digitFormat[] = "%d";

                        for(int deviceIdx = 0; deviceIdx < devicesInSnapshot.size(); deviceIdx++){
                            //auto device = deviceManager.getDevices().at(deviceIdx)
                            if(devicesInSnapshot[deviceIdx]["id"].isNull()) continue;
                            if(devicesInSnapshot[deviceIdx]["id"].as<int>() == deviceId)  
                            {
                                if(devicesInSnapshot[deviceIdx]["value"].isNull()){
                                    Serial.printf("Value is null!!\n");
                                    break;
                                }

                                bool deviceManaged = scheduleManager.isManaged(deviceId);
                                auto uom = devicesInSnapshot[deviceIdx]["uom"].isNull() ? "" : devicesInSnapshot[deviceIdx]["uom"].as<const char*>();
                                if(devicesInSnapshot[deviceIdx]["value"].is<uint16_t>()){
                                    uint16_t sourceDeviceValue = devicesInSnapshot[deviceIdx]["value"].as<uint16_t>();
                                    setDetails(string_format("%u%s%s", sourceDeviceValue, uom, deviceManaged ? " (*)" : "").c_str(), elm_messages);

                                } 
                                else if(devicesInSnapshot[deviceIdx]["value"].is<double>()){
                                    double sourceDeviceValue = devicesInSnapshot[deviceIdx]["value"].as<double>();
                                    setDetails(string_format("%02.02f%s%s", sourceDeviceValue, uom, deviceManaged ? " (*)" : "").c_str(), elm_messages);
                                }
                                else if(devicesInSnapshot[deviceIdx]["value"].is<float>()){
                                    float sourceDeviceValue = devicesInSnapshot[deviceIdx]["value"].as<float>();
                                    setDetails(string_format("%02.02f%s%s", sourceDeviceValue, uom, deviceManaged ? " (*)" : "").c_str(), elm_messages);

                                }
                                else if(devicesInSnapshot[deviceIdx]["value"].is<bool>()){    
                                    bool value = devicesInSnapshot[deviceIdx]["value"].as<bool>();
                                    setDetails(string_format("%s%s", value ? "ON" : "OFF", deviceManaged ? " (*)" : "").c_str(), elm_messages);
                                }
                                else if(devicesInSnapshot[deviceIdx]["value"].is<int>()){
                                    int sourceDeviceValue = devicesInSnapshot[deviceIdx]["value"].as<int>();
                                    setDetails(string_format("%d%s", sourceDeviceValue, deviceManaged ? " (*)" : "").c_str(), elm_messages);
                                }
                                else if(devicesInSnapshot[deviceIdx]["value"].is<short>()){
                                    short sourceDeviceValue = devicesInSnapshot[deviceIdx]["value"].as<short>();
                                    setDetails(string_format("%d%s", sourceDeviceValue, deviceManaged ? " (*)" : "").c_str(), elm_messages);
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
            _lastScrollTime = millis();
        }
    }
    //time to scroll?
    //Serial.printf("Checking if time %lu is time to scroll at %lu\n", _lastScrollTime + _scrollSpeed, millis());
    if(_lastScrollTime + _scrollSpeed > millis())
        return;

    string details = string(_details);
    //Serial.printf("Checking if there is content to scroll with length %d\n", _details.length());
    //anything to scroll?
    if(details.length() <= LCD_WIDTH)
        return;

    _lcd.setCursor(0,1);

    //Serial.printf("Checking if offset %d needs to be reset: %s\n", _offset, _offset >= LCD_WIDTH ? "Yes" : "No");
    if(_offset >= LCD_WIDTH) _offset = 0;
    else _offset++;

    int charsLeft = details.length() - _offset;

    if(charsLeft <= LCD_WIDTH){        
        //can fit rest of string
        _lcd.setCursor(0,1);
        _lcd.print(details.substr(_offset,charsLeft).c_str());
        int charsWritten = charsLeft - _offset;
        //Serial.printf("Wrote %d chars to lcd details (all)\n", charsWritten);
        
        if(LCD_WIDTH - charsWritten > _spaceBetweenText){
            //enough room to wrap around
            _lcd.print(string(_spaceBetweenText, ' ').c_str());
            int screenOffset = charsWritten + _spaceBetweenText;
            //_lcd.setCursor(screenOffset,1);
            //Serial.printf("Writing %d additional chars from beggining at position %d\n", LCD_WIDTH - screenOffset, screenOffset);
            _lcd.print(details.substr(0, LCD_WIDTH - screenOffset).c_str());
        }
    }else{
        //only enough room to print some of the text
        _lcd.print(details.substr(_offset, LCD_WIDTH).c_str());
        //Serial.printf("Wrote %d chars to lcd details (some) \n", LCD_WIDTH);
    }
    _lastScrollTime = millis();
    //Serial.printf("Updating last scroll time to %lu\n", _lastScrollTime);
    
}

void esp32_lcd::setTitle(const char *text, esp32_lcd_mode mode, bool clearLine)
{
    if(!_initialized) return;
    _mode = mode;
    memset(_title, 0, sizeof(_title));
    memcpy(_title,text,strlen(text) > 64 ? 64 : strlen(text));
    _updatePending = true;
    
}

void esp32_lcd::setDetails(const char *text, esp32_lcd_mode mode, bool clearLine)
{
    if(!_initialized) return;    

    _mode = mode;
    memset(_details, 0, sizeof(_details));
    memcpy(_details,text,strlen(text) > 64 ? 64 : strlen(text));    
        
}

void esp32_lcd::set(const char *title, const char *details, esp32_lcd_mode mode)
{
    if(!_initialized) return;
    setTitle(title, mode, false);
    setDetails(details, mode, false);
}

void esp32_lcd::clear()
{
    if(!_initialized) return;
    _lcd.setCursor(0,0);
    //_lcd.print(string(" ", LCD_WIDTH).c_str());
    _lcd.print("                ");
    _lcd.setCursor(0,1);
    //_lcd.print(string(" ", LCD_WIDTH).c_str());
    _lcd.print("                ");
    
}

void esp32_lcd::addMessage(const char *message, const char *parameter)
{
    if(!_initialized) return;
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