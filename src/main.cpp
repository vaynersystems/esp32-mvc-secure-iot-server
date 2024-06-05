#include "System/CORE/esp32_fileio.h"
#include "System/CORE/esp32_server.h"
#include "System/MODULES/MQTT/esp32_mqtt_client.hpp"
#include "System/CORE/esp32_wifi.h"
#include "System/AUTH/esp32_authentication.h"
#include "System/AUTH/esp32_sha256.h"

#include "System/MODULES/DEVICES/esp32_devices.hpp"
#include "System/MODULES/LOGGING/esp32_logging.hpp"
#include <esp_task_wdt.h>

esp32_server server;
esp32_wifi wifi;
esp32_fileio disk;
esp32_devices deviceManager;
esp32_logging logger;
esp32_mqtt_client mqtt;
DallasTemperature sensors;

TaskHandle_t* serverTaskHandle;
TaskHandle_t* deviceTaskHandle;
TaskHandle_t* mqttClientTaskHandle;

void serverTask(void* params);
void deviceTask(void* params);
void mqttClientTask(void* params);
void onShutdown();

#if CONFIG_FREERTOS_UNICORE
    #define ARDUINO_RUNNING_CORE 0
#else
    #define ARDUINO_RUNNING_CORE 1
#endif
#define REPORT_FREQUENCY 5000000
extern const int SERVER_STACK_SIZE = 1024*24; //number of words
extern const int DEVICE_MANAGER_STACK_SIZE = 1024 * 24; //number of words
extern const int MQTT_CLIENT_STACK_SIZE = 1024 * 36; //number of words
const TickType_t deviceDelay = 600 / portTICK_PERIOD_MS, serverDelay = 100 / portTICK_PERIOD_MS;
#ifdef DEBUG
int64_t lastreportServer = 0;
string freeBytesHEAPSPretty("");
string freeBytesSTACKPretty("");
#endif
//for starting and looping server task
void serverTask(void* params) {
    server.start();
    while (true) {
        server.step();
        //vTaskDelay(serverDelay);
        #ifdef DEBUG
        if(esp_timer_get_time() - lastreportServer > REPORT_FREQUENCY){
            auto stackFreeBytes = uxTaskGetStackHighWaterMark(NULL); 
            esp32_fileio::PrettyFormat((size_t)esp_get_free_heap_size(), &freeBytesHEAPSPretty);
            esp32_fileio::PrettyFormat(stackFreeBytes, &freeBytesSTACKPretty);
            
            Serial.printf("[SERVER] Free heap: %s\t stack: %s\n", freeBytesHEAPSPretty.c_str(), freeBytesSTACKPretty.c_str());
            lastreportServer = esp_timer_get_time(); 
        }
        #endif
    }

    //#ifdef DEBUG
    Serial.printf(PROGMEM("[SERVER] Shutting down\n"));
    //#endif
    vTaskDelete( NULL );
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
        vTaskDelay(deviceDelay);
    }   
    vTaskDelete( NULL );
}

void mqttClientTask(void * params){
    //initialize mqtt    
    mqtt.start();    
    while(true){
        mqtt.loop();        
        vTaskDelay(deviceDelay);
    }
    vTaskDelete( NULL );
}

void onShutdown(){
    logger.logInfo("Shutting down controller");
}

void setup() {
    
    //debug logging
    Serial.begin(115200);
    //esp32 filesystem manager
    disk.start();    
   
    // //Connect to wifi
    wifi.start();   

    // //start logger
    logger.start();

    //mqtt.start();
    //Create Server
    xTaskCreatePinnedToCore(serverTask, "secureserver", SERVER_STACK_SIZE, NULL, 2, serverTaskHandle, ARDUINO_RUNNING_CORE); 

    //Create Device Manager
    xTaskCreatePinnedToCore(deviceTask, "devicemanager",DEVICE_MANAGER_STACK_SIZE, NULL, 2, deviceTaskHandle, ARDUINO_RUNNING_CORE);

    //Create MQTT Client
    xTaskCreate(mqttClientTask, "mqttclient",MQTT_CLIENT_STACK_SIZE, NULL, tskIDLE_PRIORITY, mqttClientTaskHandle);
    
    esp_register_shutdown_handler(onShutdown);
    
    
    // WriteFile("/test.txt", "github.com");
    // ReadFile("/test.txt");

    
    
    logger.logInfo("System started");
}
int64_t lastReportMain = 0;
string freeBytesHEAPPretty = "", freeBytesSTACKmPretty = "";
void loop() {   
    
    if(esp_timer_get_time() - lastReportMain > REPORT_FREQUENCY){        
        
        auto stackFreeBytes = uxTaskGetStackHighWaterMark(NULL); 
        esp32_fileio::PrettyFormat((size_t)esp_get_free_heap_size(), &freeBytesHEAPPretty);
        esp32_fileio::PrettyFormat(stackFreeBytes, &freeBytesSTACKmPretty);
        
        Serial.printf("[MAIN] Free heap: %s\t stack: %s\n",
            freeBytesHEAPPretty.c_str(), 
            freeBytesSTACKmPretty.c_str()
        );
        lastReportMain = esp_timer_get_time(); 
    }
}