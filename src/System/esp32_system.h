#include "CORE/esp32_fileio.hpp"
#include "CORE/esp32_server.hpp"
#include "MODULES/MQTT/esp32_mqtt_client.hpp"
#include "CORE/esp32_wifi.h"
#include "AUTH/esp32_authentication.hpp"
#include "AUTH/esp32_sha256.h"

#include "MODULES/DEVICES/esp32_devices.hpp"
#include "MODULES/SCHEDULING/esp32_scheduling_manager.hpp"
#include "MODULES/LOGGING/esp32_logging.hpp"

#include "MODULES/LCD/esp32_lcd.hpp"
#include <esp_task_wdt.h>
#include <semphr.h>

esp32_server server;
esp32_wifi wifi;
esp32_fileio disk;
esp32_devices deviceManager;
esp32_scheduling_manager scheduleManager;
esp32_logging logger;
esp32_mqtt_client mqtt;
esp32_pin_manager pinManager;

TaskHandle_t* serverTaskHandle;
TaskHandle_t* lcdTaskHandle;
TaskHandle_t* deviceTaskHandle;
TaskHandle_t* mqttClientTaskHandle;

//QueueHandle_t xMutex;
SemaphoreHandle_t lcdMutex;

void serverTask(void* params);
void lcdTask(void* params);
void deviceTask(void* params);
void mqttClientTask(void* params);
void onShutdown();

#if CONFIG_FREERTOS_UNICORE
    #define ARDUINO_RUNNING_CORE 0
#else
    #define ARDUINO_RUNNING_CORE CONFIG_ARDUINO_RUNNING_CORE
#endif
#define REPORT_FREQUENCY 5000000 // 5 seconds
// DO NOT LOWER THESE. Components will begin to malfunction causing crashes.
// SET_LOOP_TASK_STACK_SIZE(1024*16)
extern const int SERVER_STACK_SIZE = 1024*24; 
extern const int LCD_STACK_SIZE = 1024*8;
extern const int DEVICE_MANAGER_STACK_SIZE = 1024 * 24; 
extern const int MQTT_CLIENT_STACK_SIZE = 1024 * 36;
const TickType_t deviceDelay = 200 / portTICK_PERIOD_MS, serverDelay = 50 / portTICK_PERIOD_MS, lcdDelay = 400 / portTICK_PERIOD_MS;

//for starting and looping server task
void serverTask(void* params) {
    server.start();
    while (true) {
        if(xSemaphoreTake( lcdMutex, serverDelay ) == pdTRUE)
        {
            server.step();xSemaphoreGive(lcdMutex);            
        }
        vTaskDelay(serverDelay);         
               
    }

    //#ifdef DEBUG
    Serial.printf(PROGMEM("[SERVER] Shutting down\n"));
    //#endif
    vTaskDelete( NULL );
}

void lcdTask(void* params){
    lcd.begin(PIN_SDA, PIN_SCL);
    lcd.clear();
    lcd.setTitle(string_format("ESP32 IoT v%s", FIRMWARE_VERSION).c_str());
    lcd.setDetails(string_format("Built %s",FIRMWARE_DATE).c_str());
    while(true){
        if(xSemaphoreTake( lcdMutex, lcdDelay ) == pdTRUE){
            lcd.loop();       
            xSemaphoreGive(lcdMutex); 
        }
        vTaskDelay(lcdDelay);
    }    
}

unsigned long deviceLoopTime = 0;
void deviceTask(void* params) {
    
    deviceManager.onInit();
    scheduleManager.onInit();

    while(true){
        if(xSemaphoreTake( lcdMutex, deviceDelay) == pdTRUE){
            scheduleManager.onLoop();        
            deviceManager.onLoop();
            xSemaphoreGive(lcdMutex);
        }
        vTaskDelay(deviceDelay);
    }   
    vTaskDelete( NULL );
}

void mqttClientTask(void * params){
    //initialize mqtt    
    mqtt.start();
    if(mqtt.enabled()) // do not use all this RAM if its disabled     
        while(true){
            mqtt.loop();        
            vTaskDelay(deviceDelay);
        }
    vTaskDelete( NULL );
}

void onShutdown(){
    logger.logInfo("Shutting down controller");
}


void esp32_system_start(){
    lcdMutex = xSemaphoreCreateMutex();
    Serial.printf("Starting esp32-mvc-controller firmware (v %s)\n", FIRMWARE_VERSION);
    // Initialize LCD    
    xTaskCreatePinnedToCore(lcdTask, "lcd", LCD_STACK_SIZE, NULL, 1, lcdTaskHandle, ARDUINO_RUNNING_CORE); 
    //xTaskCreate(lcdTask, "lcd",LCD_STACK_SIZE, NULL, 2, lcdTaskHandle);
    
    //debug logging
    lcd.setDetails(string_format("Serial logging (%d baud)..",115200).c_str());
    Serial.begin(115200);
    
    //esp32 filesystem manager
    lcd.setDetails("Starting disk management..");
    disk.start();    

    // //Connect to wifi
    lcd.setDetails("Connecting to Wifi..");
    wifi.start();   
    
    logger.start();    
    
    lcd.setDetails("Init Server");
    //Create Server
    xTaskCreatePinnedToCore(serverTask, "secureserver", SERVER_STACK_SIZE, NULL, 2 | portPRIVILEGE_BIT, serverTaskHandle, ARDUINO_RUNNING_CORE); 
    
    lcd.setDetails("Init Device Manager");
    //Create Device Manager
    xTaskCreatePinnedToCore(deviceTask, "devicemanager",DEVICE_MANAGER_STACK_SIZE, NULL, 2, deviceTaskHandle, ARDUINO_RUNNING_CORE);
    

    lcd.setDetails("Init MQTT");
    //Create MQTT Client
    xTaskCreate(mqttClientTask, "mqttclient",MQTT_CLIENT_STACK_SIZE, NULL, tskIDLE_PRIORITY, mqttClientTaskHandle);
    //example use of psram
    //byte* psram = (uint8_t*)ps_calloc(100000, sizeof(uint32_t));
    esp_register_shutdown_handler(onShutdown);

    logger.logInfo(string_format("System started @ %s\n", getCurrentTime().c_str()));
    
    lcd.set("System started",getCurrentTime().c_str());

}