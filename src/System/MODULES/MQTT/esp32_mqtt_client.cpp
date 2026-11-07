#include "esp32_mqtt_client.hpp"
#include "string_helper.h"



void esp32_mqtt_client::start()
{
    StaticJsonDocument<2048> systemConfig;
    esp32_config::getConfigSection("system", &systemConfig);
    if(systemConfig["mqtt"].isNull()) return; //no init, not configured
    if(systemConfig["mqtt"]["enabled"].isNull() || !systemConfig["mqtt"]["enabled"].as<bool>() == true)
        return; //not configured

    _enabled = true;
    if(!_enabled) return; //do not initialize if disabled
    _subscribeEnabled = systemConfig["mqtt"]["subscribeEnabled"].as<bool>();
    _insecureMode = systemConfig["mqtt"]["insecure"].isNull() || systemConfig["mqtt"]["insecure"].as<bool>();

    if(_insecureMode)
        networkSecure.setInsecure();
    // The core's default TLS handshake timeout is 120 s; an unreachable broker would hold
    // this task (and a TLS allocation) that long per attempt.
    networkSecure.setHandshakeTimeout(MQTT_HANDSHAKE_TIMEOUT_S);

    _brokerUri = systemConfig["mqtt"]["broker"].isNull() ? "test.mosquitto.org" :  systemConfig["mqtt"]["broker"].as<const char*>();
    _port = systemConfig["mqtt"]["port"].isNull() ? 8883 : systemConfig["mqtt"]["port"].as<int>();
    _hostname = systemConfig["hostname"].as<const char *>();
    #ifdef DEBUG
    Serial.printf("Setting MQTT broker to %s on port %d.\n", _brokerUri.c_str(), _port);
    #endif
    if(_port == 8883){ //load certs for encrypted traffic
        DynamicJsonDocument serverConfig(1024);
        esp32_config::getConfigSection("server", &serverConfig);        
        esp32_cert_base* certManager;
        if(serverConfig["certificates"]["source"].isNull() || 
            iequals(serverConfig["certificates"]["source"].as<const char*>(),"nvs",3))
                certManager = new esp32_cert_nvs();        
        
        else
            certManager = new esp32_cert_fs();        
        
        //initialize certificates
        certManager->loadCertificates(); 
        networkSecure.setCertificate((const char*) certManager->getCert()->getCertData());
        networkSecure.setPrivateKey((const char*) certManager->getCert()->getPKData());
        client = PubSubClient(networkSecure);
    } else{ //unecrypted    
        client = PubSubClient(networkInsecure);
    }

    client.setKeepAlive(60);
    
    client.setServer(_brokerUri.c_str(),_port);  
    xSemaphoreTake(_queueLock, portMAX_DELAY);
    _publishList.clear();
    xSemaphoreGive(_queueLock);
    _started = true; 
    #ifdef DEBUG
    Serial.println("MQTT Client initialization completed");
    #endif
}

bool esp32_mqtt_client::connect()
{
    if(!_started) {
        
        #ifdef DEBUG
        Serial.println("Failed to connect. MQTT Client is not started.");
        #endif
        return false;
        
    }
    // Back off after a failure: every attempt is a full TLS handshake that competes with the
    // web server for heap and the network stack. Retrying every loop pass (200 ms) made web
    // pages load partially while the broker was unreachable.
    if(_retryDelayMs > 0 && millis() - _lastAttemptMs < _retryDelayMs)
        return false;
    _lastAttemptMs = millis();

    // PubSubClient opens the network connection itself; opening it here as well cost a second
    // handshake per attempt.
    if(_port == 8883){
        if(networkSecure.connected()) networkSecure.stop();
    } else{
        if(networkInsecure.connected()) networkInsecure.stop();
        #ifdef DEBUG
        Serial.printf("[MQTT] Connecting to INSECURE network %s on port %d\n", _brokerUri.c_str(),_port);
        #endif
    }
    bool connected = client.connect(_brokerUri.c_str());
    if(connected){
        if(_retryDelayMs > 0) Serial.printf("[MQTT] connected to %s:%d\n", _brokerUri.c_str(), _port);
        _retryDelayMs = 0;
    } else {
        _retryDelayMs = _retryDelayMs == 0 ? MQTT_RETRY_MIN_MS : _retryDelayMs * 2;
        if(_retryDelayMs > MQTT_RETRY_MAX_MS) _retryDelayMs = MQTT_RETRY_MAX_MS;
        Serial.printf("[MQTT] could not connect to %s:%d (state %d); retrying in %lu s\n",
            _brokerUri.c_str(), _port, client.state(), _retryDelayMs / 1000);
    }
    return connected;
}

/// @brief Durable delivery. Message will wait in queue until it is confirmed as delivered to host
void esp32_mqtt_client::loop(){
    if(!_enabled || !_started) {
        Serial.printf("MQTT Disabled. Quitting");
        return;
    }
    // Take the whole queue under the lock, then publish without it, so the device task can
    // keep queueing while a (slow) broker connection is in progress.
    vector<pair<string, string>> batch;
    xSemaphoreTake(_queueLock, portMAX_DELAY);
    batch.swap(_publishList);
    xSemaphoreGive(_queueLock);
    if(batch.empty())
        return;
    if(!isConnected() && !connect()){
        requeue(batch); // keep it for the next attempt
        return;
    }
    #ifdef DEBUG
    Serial.printf("Processing %d publications\n", batch.size());
    #endif
    // Newest first, as before. Count what went out so a failure only requeues the rest
    // (previously, messages already sent were kept and sent again).
    size_t sent = 0;
    for(auto it = batch.rbegin(); it != batch.rend(); ++it){
        bool result = client.publish(it->first.c_str(), it->second.c_str());
        #ifdef DEBUG
        Serial.printf("Publishing to topic %s %s. Result: %s\n", it->first.c_str(), it->second.c_str(), result ? "sucessfull" : "failed");
        #endif
        logger.logDebug(string_format("Publishing. Topic: [%s] Value [%s] - Result: %s", it->first.c_str(), it->second.c_str(), result ? "suceeded" : "failed"));
        if(!result) break;
        sent++;
    }
    if(sent < batch.size()){
        batch.resize(batch.size() - sent); // the unsent ones are at the front
        requeue(batch);
        return;
    }
    //TODO: figure out why this prevents server from responding to requests.
    //  possibly timers are being reallocated
    if(_subscribeEnabled)
        client.loop();
}



/// @brief Puts unsent messages back ahead of anything queued meanwhile, keeping the newest
/// MQTT_MAX_QUEUED.
void esp32_mqtt_client::requeue(vector<pair<string, string>>& unsent){
    xSemaphoreTake(_queueLock, portMAX_DELAY);
    _publishList.insert(_publishList.begin(), unsent.begin(), unsent.end());
    if(_publishList.size() > MQTT_MAX_QUEUED)
        _publishList.erase(_publishList.begin(), _publishList.end() - MQTT_MAX_QUEUED);
    xSemaphoreGive(_queueLock);
}

void esp32_mqtt_client::disconnect()
{
    _port == 8883 ? networkSecure.stop() : networkInsecure.stop();
}
//TODO: This functionality is not yet implemented/verified
void esp32_mqtt_client::subscribe(const char *topic, void (*callback)(char* topic, uint8_t* payload, unsigned int size))
{
    logger.logDebug(string_format("Subscribed to %s\n", topic));
    client.subscribe(topic,2);
    client.setCallback(callback);
}


bool esp32_mqtt_client::publish(const char *topic, const char* data)
{
    if(!_enabled || !_started) return false;
    // While the broker is unreachable the queue would otherwise grow without limit.
    xSemaphoreTake(_queueLock, portMAX_DELAY);
    if(_publishList.size() >= MQTT_MAX_QUEUED)
        _publishList.erase(_publishList.begin()); // drop the oldest
    _publishList.push_back(pair<string, string>(topic, data));
    xSemaphoreGive(_queueLock);
    logger.logDebug(string_format("Published %s to topic %s", data, topic),esp32_log_type::device);
    #ifdef DEBUG
    Serial.printf("Added message %s to topic %s\n", data, topic);
    #endif
   
    return true;   
}

void esp32_mqtt_client::callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}