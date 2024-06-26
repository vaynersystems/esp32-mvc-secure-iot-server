#include "esp32_logging.hpp"
#include "string_helper.h"
#include <System/CORE/esp32_fileio.h>
#include <loopback_stream.h>
#include <system_helper.h>
#include <regex>
extern esp32_file_system filesystem;
const char* loggingSection = "logging";
void esp32_logging::start()
{
    logTypes[esp32_log_type::syslog] = "SYSLOG";
    logTypes[esp32_log_type::device] = "DEVICE";
    logTypes[esp32_log_type::snapshot] = "SNAPSHOT";
    logTypes[esp32_log_type::auth] = "AUTH";
    logEntryTypes[info] = "INFO";
    logEntryTypes[warning] = "WARNING";
    logEntryTypes[error] = "ERROR";
    logEntryTypes[debug] = "DEBUG";

    StaticJsonDocument<1024> systemConfig;
    esp32_config::getConfigSection("system", &systemConfig);   
    if(!systemConfig[loggingSection].isNull()){
        if(!systemConfig[loggingSection]["retention"].isNull() )
            _retentionDays = systemConfig[loggingSection]["retention"].as<int>();
        
        if(!systemConfig[loggingSection]["level"].isNull() )
            _loggingLevel = systemConfig[loggingSection]["level"].as<esp32_log_level>();

        if(!systemConfig[loggingSection]["location"].isNull() ){
            _location = systemConfig[loggingSection]["location"].as<esp32_drive_type>();
            if(filesystem.driveCount() <= 1 && _location != dt_SPIFFS)
                _location = dt_SPIFFS; // default to SPIFFS if configured volume is not found
            #ifdef DEBUG
            if(_location == dt_SD){
                Serial.println("Setting SD as log location");
            } else{                
                Serial.println("Setting SPIFFS as log location");
            }
            #endif
        }
            
        
    }

    rotateLogs(syslog);
    rotateLogs(device);
    rotateLogs(snapshot);    
}



bool esp32_logging::logInfo(string message, esp32_log_type logType)
{
    return logInfo(message.c_str(),logType);
}

bool esp32_logging::logError(string error, esp32_log_type logType)
{
    return logError(error.c_str(), logType);
}

bool esp32_logging::logInfo(const char *message, esp32_log_type logType)
{
    return log(message, logType, esp32_log_level::info);
}

bool esp32_logging::logError(const char *error, esp32_log_type logType)
{
    return log(error, logType, esp32_log_level::error);
}

bool esp32_logging::logWarning(string warning, esp32_log_type logType)
{
    return log(warning, logType, esp32_log_level::warning);
}

bool esp32_logging::logWarning(const char *warning, esp32_log_type logType)
{
    return log(warning, logType, esp32_log_level::warning);
}

bool esp32_logging::logDebug(string message, esp32_log_type logType)
{
    return log(message, logType, esp32_log_level::debug);
}

bool esp32_logging::logDebug(const char *message, esp32_log_type logType)
{
    return log(message, logType, esp32_log_level::debug);
}

bool esp32_logging::logSnapshot(JsonObject snapshot)
{
    if(snapshot.isNull() || snapshot.size() == 0) return false;
    string snapshotString = "";
    serializeJson(snapshot, snapshotString);
    //auto logLocation = fs();

    string filename = getLogFilename(esp32_log_type::snapshot);  
    if(filename.length() == 0) return false; 
    
    auto fileInfo = esp32_file_info_extended(filename.c_str());

    //Serial.printf("Setting snapshot logfile location to %s\n", fileInfo.fullyQualifiedPath().c_str());
    
    //Serial.printf("Log %s %s\n", filename.c_str(), fileInfo.exists() ? "found" : "not found");
  
    if(! fileInfo.exists()){ //new log file
        //run cleanup
        rotateLogs(esp32_log_type::snapshot);
        esp32_fileio::UpdateFile(filename.c_str(), string_format("[\n\t%s\n]",snapshotString.c_str()).c_str(),true);
       
    } else{
        esp32_fileio::UpdateFile(filename.c_str(), string_format("\n\t,%s\n]",snapshotString.c_str()).c_str(),true, -2);        
    }
    
    return true;
}

bool esp32_logging::log(string message, esp32_log_type logType, esp32_log_level entryType)
{
    return log(message.c_str(),logType, entryType);
}

int esp32_logging::rotateLogs(esp32_log_type logType)
{
    vector<esp32_file_info_extended> files;
    auto drive = filesystem.getDisk(_location); //relying on the simple fact that spiffs is mounted at 0, SD at 1, just as enum, dicey!   
    auto logsFound = drive->search(files,PATH_LOGGING_ROOT, logTypes[logType]);
    
    if(logsFound == 0 || _retentionDays == 0) return 0; //configured for no rotation 
    int deleted = 0;
    //tm now = getDate(); 
    time_t now;
    time(&now);// = mktime(&now);
    if(logsFound < 0) 
    {
        //Serial.printf("Error occured listing files: %d\n", logsFound);
        return 0;
    }
    if (logsFound == 0)
        return deleted;

    for(int idx = 0; idx < files.size();idx++){
        if(now - files[idx].lastWrite() > _retentionDays * 24 * 60 * 60){
            logInfo(string_format("Removing log file %s due to retention policy. It is %d days old.",
                files[idx].name().c_str(), (now - files[idx].lastWrite())/(60*60*24)));
            drive->remove(files[idx].fullyQualifiedPath().c_str());
            deleted++;
        } else {
            //Serial.printf("Keeping log file %s since it is %d days old\n", log["name"].as<const char *>(), (now - lastWrite)/(60*60*24));
        }    
    }
    
    return deleted;
}

void esp32_logging::removeAllLogs()
{
    vector<esp32_file_info> files;
    auto drive = filesystem.getDisk(_location); //relying on the simple fact that spiffs is mounted at 0, SD at 1, just as enum, dicey!
    auto logsFound = drive->search(files,PATH_LOGGING_ROOT);

    if(logsFound == 0) return;
    for(int idx = 0; idx < files.size();idx++){
        drive->remove(files[idx].fullyQualifiedPath().c_str());
    }

}

string esp32_logging::getLogFilename(esp32_log_type logType)
{
    struct tm timeinfo = getDate();
    auto drive = filesystem.getDisk(_location); //relying on the simple fact that spiffs is mounted at 0, SD at 1, just as enum, dicey!
   
    if(timeinfo.tm_year == 70)
        return ""; // clock not initialized
    return string_format("/%s%s/%s_%04d-%02d-%02d.log",
        //_location == drive_SPIFFS ? "/spiffs" : "/sd",
        drive->label(), PATH_LOGGING_ROOT, logTypes[logType], timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);

}

bool esp32_logging::log(const char *message, esp32_log_type log, esp32_log_level entryType)
{
    if(_loggingLevel < entryType) return false;
    auto drive = filesystem.getDisk(_location); //relying on the simple fact that spiffs is mounted at 0, SD at 1, just as enum, dicey!
   
   // auto logLocation = fs();

    //auto logLocation = _location == drive_SPIFFS ? (FS)SPIFFS : (FS)SD;
    string filename = getLogFilename(log);  
    struct tm timeinfo = getDate();
    if(filename.length() == 0) return false; 
    //Serial.printf("Setting logfile location to %s\n", filename.c_str());
    
    string const esacped = regex_replace( message, std::regex( "\"" ), "\\\"" );

    bool logFileExists =  drive->exists(filename.c_str());
    //Serial.printf("Log %s %s\n", filename.c_str(), logFileExists ? "found" : "not found");
  
    if(!logFileExists){ //new log file
        //Serial.println("Log file not found. Creating");
        //esp32_fileio::CreateFile(filename.c_str());
        File logFile = drive->open(filename.c_str(),"w",true);
        logFile.printf("[\n\t {\"time\":\"%02d:%02d:%02d\", \"type\": \"%s\", \"message\": \"%s\"}\n]",
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec,
            logEntryTypes[entryType],
            esacped.c_str()
        );        
        logFile.close();
        //run cleanup
        rotateLogs(log);
        
    } else{
        //Serial.println("Log file found. Opening");
        File logFile = drive->open(filename.c_str(),"r+w");
        if(!logFile) return false;      
        int fileSize = logFile.size();
        int seekPos = fileSize > 0 ? fileSize - 1 : 0;
        //Serial.printf("Seeking from position %d to position %d of %d in daily %s file .\n", logFile.position(), seekPos, fileSize, logTypes[log]);
        bool seekWorked = logFile.seek(seekPos, SeekMode::SeekSet);
        if(!seekWorked){
            #ifdef DEBUG
            Serial.println("Logging - Failed to seek. Aborting!");
            #endif
            return false;
        }
        logFile.printf("\t, {\"time\":\"%02d:%02d:%02d\", \"type\": \"%s\" , \"message\": \"%s\"}\n]",
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec,
            logEntryTypes[entryType],
            esacped.c_str()
        ); 
        logFile.close();
    }
    
    return true;
}
