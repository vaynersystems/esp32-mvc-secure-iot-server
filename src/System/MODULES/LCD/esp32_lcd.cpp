#include "esp32_lcd.hpp"

void esp32_lcd::begin(int sda, int scl)
{
    _lcd.begin(LCD_WIDTH,2,LCD_5x8DOTS, sda, scl);
    _lcd.clear();        
    //Serial.println("Initialized LCD");
}

void esp32_lcd::loop()
{
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
        
        if(LCD_WIDTH - charsWritten > _spaceBetweenMessage){
            //enough room to wrap around
            _lcd.print(string(_spaceBetweenMessage, ' ').c_str());
            int screenOffset = charsWritten + _spaceBetweenMessage;
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

void esp32_lcd::setTitle(const char *text)
{
    _title = text;
    _lcd.setCursor(0,0);
    _lcd.print(text);
}

void esp32_lcd::setDetails(const char *text)
{
    _details = text;
    _offset = 0;
    _lcd.setCursor(0,1);    
    _lcd.print(text);
    _lastScrollTime = millis();
}

void esp32_lcd::set(const char *title, const char *details)
{
    _lcd.clear();
    setTitle(title);
    setDetails(details);
}

void esp32_lcd::clear()
{
    _lcd.clear();
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