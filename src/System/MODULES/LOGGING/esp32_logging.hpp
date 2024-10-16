#ifndef _ESP32_LOGGER_H
#define _ESP32_LOGGER_H
#include "Arduino.h"
#include <map>
#include "ArduinoJson.h"
#include <SD.h>
#include <System/Config.h>
#include <System/CORE/esp32_config.h>
#include "esp32_filesystem.hpp"
#include "System/MODULES/LCD/esp32_lcd.hpp"
extern esp32_lcd lcd;

using namespace std;
enum esp32_log_type{
    syslog = 0,
    device = 1,
    snapshot = 2,
    auth = 3
};

enum esp32_log_level{
    disabled = 1,
    error = 2,
    warning = 3,
    info = 4,
    debug = 5
};

//syslog
class esp32_logging{
public:

    void start();
    
    bool logInfo(string message, esp32_log_type logType = syslog);
    bool logInfo(const char* message, esp32_log_type logType = syslog);
    bool logError(string error, esp32_log_type logType = syslog);    
    bool logError(const char* error, esp32_log_type logType = syslog);
    bool logWarning(string warning, esp32_log_type logType = syslog);    
    bool logWarning(const char* warning, esp32_log_type logType = syslog);
    bool logDebug(string message, esp32_log_type logType = syslog);    
    bool logDebug(const char* message, esp32_log_type logType = syslog);

    bool logSnapshot(JsonObject snapshot);
    bool log(string message, esp32_log_type logType = syslog, esp32_log_level entryType = info);
    bool log(const char* message, esp32_log_type logType = syslog, esp32_log_level entryType = info);
    int rotateLogs(esp32_log_type logType);
    //static size_t size();

    void removeAllLogs();

    esp32_drive_type location(){
        return _location;
    };

private:
    string getLogFilename(esp32_log_type logType);
    //void ensureLogDirExists();
    
    const char * _logPrefix = "SYSLOG";
    bool _useDate = true;
    int _retentionDays = 365;
    esp32_log_level _loggingLevel;
    esp32_drive_type _location;
    std::map<esp32_log_type, const char *> logTypes;
    std::map<esp32_log_level, const char *> logEntryTypes;
};

#endif

