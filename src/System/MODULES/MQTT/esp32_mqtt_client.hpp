#ifndef _ESP32_MQTT_CLIENT_H
#define _ESP32_MQTT_CLIENT_H
#include <WiFiClientSecure.h>
#include "PubSubClient.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "System/MODULES/LOGGING/esp32_logging.hpp"
#include "System/CORE/esp32_config.hpp"
#include "System/CORE/esp32_server.hpp"
extern esp32_logging logger;
extern esp32_server server;

#ifndef MQTT_RETRY_MIN_MS
#define MQTT_RETRY_MIN_MS 5000
#endif
#ifndef MQTT_RETRY_MAX_MS
#define MQTT_RETRY_MAX_MS 300000
#endif
#ifndef MQTT_HANDSHAKE_TIMEOUT_S
#define MQTT_HANDSHAKE_TIMEOUT_S 10
#endif
#ifndef MQTT_MAX_QUEUED
#define MQTT_MAX_QUEUED 50
#endif

class esp32_mqtt_client{
    public:
        void start(); 
        void loop();
        bool publish(const char * topic, const char* data);

        void subscribe(const char * topic, void (*callback)(char* topic, uint8_t* payload, unsigned int length));

        inline bool enabled() { return _enabled;}
    private:
        bool connect();
        void disconnect();

        bool isConnected(){
            return (_port == 8883 ? networkSecure.connected() : networkInsecure.connected());
        }
        WiFiClientSecure networkSecure;
        WiFiClient networkInsecure;

        void callback(char* topic, byte* payload, unsigned int length);

        bool _insecureMode;
        bool _started = false;
        bool _enabled = false;
        bool _subscribeEnabled = false;
        string _brokerUri;
        string _hostname;
        int _port;
        int _timeout;
        unsigned long _lastAttemptMs = 0;
        unsigned long _retryDelayMs = 0; // 0 = connect right away

        // publish() runs on the device task and loop() on the MQTT task; _queueLock guards
        // _publishList. It is never held across network calls.
        vector<pair<string, string>> _publishList;
        SemaphoreHandle_t _queueLock = xSemaphoreCreateMutex();
        void requeue(vector<pair<string, string>>& unsent);


        PubSubClient client;
        // TimerHandle_t mqttReconnectTimer;
        // TimerHandle_t wifiReconnectTimer;

};


#endif

