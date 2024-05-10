#include "System/AUTH/cert.h"
#include "System/AUTH/key.h"

#include "System/CORE/esp32_fileio.h"
#include "System/CORE/esp32_server.h"
#include "System/CORE/esp32_wifi.h"
#include "System/AUTH/esp32_authentication.h"
#include "System/AUTH/esp32_sha256.h"

//TODO: store cert in SPIFFS
//#define CERT_FILE_CER   "/PRI/CERT.DER"
//#define CERT_FILE_KEY   "/PRI/CERM.KEY.DER"

public_cert pubCert = public_cert();
private_cert priCert = private_cert();
static SSLCert  cert = 
SSLCert((unsigned char*)pubCert.esp32_vaynersystems_com_der, pubCert.esp32_vaynersystems_com_der_len,
(unsigned char*)priCert.esp32_vaynersystems_com_key, priCert.esp32_vaynersystems_com_key_der_len);
esp32_server server(&cert);
esp32_wifi wifi;
esp32_fileio disk;

TaskHandle_t* task;
void serverTask(void* params);

#if CONFIG_FREERTOS_UNICORE
    #define ARDUINO_RUNNING_CORE 0
#else
    #define ARDUINO_RUNNING_CORE 1
#endif
extern const int SERVER_STACK_SIZE = 1024*24;
#ifdef DEBUG
unsigned long lastreport = millis();
String freeBytesHEAPSPretty("");
String freeBytesSTACKPretty("");
#endif
//for starting and looping server task
void serverTask(void* params) {
    server.start();
    while (true)         
        server.step();

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

void setup() {
    //logging
    Serial.begin(115200);
    //spiffs
    disk.start();    
    //Connect to wifi
    wifi.start();     
    //Create Server
    xTaskCreatePinnedToCore(serverTask, "secureserver", SERVER_STACK_SIZE, NULL, 1, task, ARDUINO_RUNNING_CORE); 
}


void loop() {   


}
