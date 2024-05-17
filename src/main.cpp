#include "System/AUTH/cert.h"
#include "System/AUTH/key.h"

#include "System/CORE/esp32_fileio.h"
#include "System/CORE/esp32_server.h"
#include "System/CORE/esp32_wifi.h"
#include "System/AUTH/esp32_authentication.h"
#include "System/AUTH/esp32_sha256.h"

#include "SD.h"
#include "SPI.h"
#include "System/MODULES/DEVICES/esp32_devices.hpp"
#include "System/MODULES/LOGGING/esp32_logging.hpp"
// #define LED_PIN 23
// File myFile;
// const int CS = 15;

// void WriteFile(const char * path, const char * message){
//   // open the file. note that only one file can be open at a time,
//   // so you have to close this one before opening another.
//   myFile = SD.open(path, FILE_WRITE);
//   // if the file opened okay, write to it:
//   if (myFile) {
//     Serial.printf("Writing to %s ", path);
//     myFile.println(message);
//     myFile.close(); // close the file:
//     Serial.println("completed.");
//   } 
//   // if the file didn't open, print an error:
//   else {
//     Serial.println("error opening file ");
//     Serial.println(path);
//   }
// }


// void ReadFile(const char * path){
//   // open the file for reading:
//   myFile = SD.open(path);
//   if (myFile) {
//      Serial.printf("Reading file from %s\n", path);
//      // read from the file until there's nothing else in it:
//     while (myFile.available()) {
//       Serial.write(myFile.read());
//     }
//     myFile.close(); // close the file:
//   } 
//   else {
//     // if the file didn't open, print an error:
//     Serial.println("error opening test.txt");
//   }
// }


esp32_server server;
esp32_wifi wifi;
esp32_fileio disk;
esp32_devices deviceManager;
esp32_logging logger;
DallasTemperature sensors;

TaskHandle_t* serverTaskHandle;
TaskHandle_t* deviceTaskHandle;
void serverTask(void* params);
void deviceTask(void* params);

#if CONFIG_FREERTOS_UNICORE
    #define ARDUINO_RUNNING_CORE 0
#else
    #define ARDUINO_RUNNING_CORE 1
#endif
extern const int SERVER_STACK_SIZE = 1024*24;
extern const int DEVICE_MANAGER_STACK_SIZE = 1024 * 16;
const TickType_t xDelay = 600 / portTICK_PERIOD_MS;
#ifdef DEBUG
unsigned long lastreport = millis();
String freeBytesHEAPSPretty("");
String freeBytesSTACKPretty("");
#endif
//for starting and looping server task
void serverTask(void* params) {
    server.start();
    while (true) {
        server.step();
         //vTaskDelay(xDelay);
    }

    #ifdef DEBUG
    if(millis() - lastreport > 1000){
        auto stackFreeBytes = uxTaskGetStackHighWaterMark(NULL); 
        esp32_fileio::PrettyFormat((size_t)esp_get_free_heap_size(), &freeBytesHEAPSPretty);
        esp32_fileio::PrettyFormat(stackFreeBytes, &freeBytesSTACKPretty);
        
        Serial.printf("Free heap: %s\t stack: %s\n", freeBytesHEAPSPretty.c_str(), freeBytesSTACKPretty.c_str());
        lastreport = millis(); 
    }
    #endif
}

unsigned long deviceLoopTime = 0;
void deviceTask(void* params) {
    
    deviceManager.onInit();

    while(true){
        // #ifdef DEBUG
        // deviceLoopTime = millis();
        // #endif
        deviceManager.onLoop();

        // #ifdef DEBUG
        // Serial.printf("Device Loop took %lu ms.\n", millis() - deviceLoopTime);
        // #endif
        vTaskDelay(xDelay);
    }   

}

void setup() {
    
    //debug logging
    Serial.begin(115200);
    //spiffs
    disk.start();    
   
    //Connect to wifi
    wifi.start();   

    //start logger
    logger.init();
    //Create Server
    xTaskCreatePinnedToCore(serverTask, "secureserver", SERVER_STACK_SIZE, NULL, 2, serverTaskHandle, ARDUINO_RUNNING_CORE); 

    //Create Device Manager
    xTaskCreatePinnedToCore(deviceTask, "devicemanager",DEVICE_MANAGER_STACK_SIZE, NULL, 2, deviceTaskHandle, ARDUINO_RUNNING_CORE);
    //pinMode(LED_PIN, OUTPUT);

    // if (!SD.begin(CS)) {
    //     Serial.println("initialization failed!");
    //     return;
    // }
    // WriteFile("/test.txt", "ElectronicWings.com");
    // ReadFile("/test.txt");
    
    logger.logInfo(string_format("System started at %s", getCurrentTime().c_str()).c_str());
}

void loop() {   
    
}
