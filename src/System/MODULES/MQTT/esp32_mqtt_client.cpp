#include "esp32_mqtt_client.hpp"
#include "string_helper.h"



void esp32_mqtt_client::start()
{
    StaticJsonDocument<2048> systemConfig;
    esp32_config::getConfigSection("system", &systemConfig);
    if(systemConfig["mqtt"].isNull()) return; //no init, not configured
    if(systemConfig["mqtt"]["enabled"].isNull() || !systemConfig["mqtt"]["enabled"].as<bool>() == true)
        return; //not configured

    _enabled = systemConfig["mqtt"]["enabled"].as<bool>();
    if(!_enabled) return; //do not initialize if disabled
    _subscribeEnabled = systemConfig["mqtt"]["subscribeEnabled"].as<bool>();
    _insecureMode = systemConfig["mqtt"]["insecure"].isNull() || systemConfig["mqtt"]["insecure"].as<bool>();

    if(_insecureMode)
        networkSecure.setInsecure();

    _brokerUri = systemConfig["mqtt"]["broker"].isNull() ? "test.mosquitto.org" :  systemConfig["mqtt"]["broker"].as<const char*>();
    _port = systemConfig["mqtt"]["port"].isNull() ? 8883 : systemConfig["mqtt"]["port"].as<int>();
    _hostname = systemConfig["hostname"].as<const char *>();
    #ifdef DEBUG
    Serial.printf("Setting MQTT broker to %s on port %d.\n", _brokerUri.c_str(), _port);
    #endif
    if(_port == 8883){ //load certs for encrypted traffic
        StaticJsonDocument<1024> serverConfig;
        esp32_config::getConfigSection("server", &serverConfig);        
        esp32_cert_base* certManager;
        if(serverConfig["certificates"]["source"].isNull() || 
            iequals(serverConfig["certificates"]["source"].as<const char*>(),"nvs",3))
                certManager = new esp32_cert_nvs();        
        
        else
            certManager = new esp32_cert_spiffs();        
        
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
    //client.setServer("test.mosquitto.org",8883);    
    _started = true; 
    //client.begin(_brokerUri.c_str(), network);
    
    _publishList.clear();
    #ifdef DEBUG
    Serial.println("MQTT Client initialization completed");
    #endif
}

bool esp32_mqtt_client::connect()
{
    if(!_started) {
        //start();
        #ifdef DEBUG
        Serial.println("Failed to connect. MQTT Client is not started.");
        #endif
        
    }
    if(_port == 8883){
        if(networkSecure.connected()) networkSecure.stop();
            networkSecure.connect(_brokerUri.c_str(),_port);
    } else{
        if(networkInsecure.connected()) networkInsecure.stop();
        #ifdef DEBUG
        Serial.printf("[MQTT] Connecting to INSECURE network %s on port %d\n", _brokerUri.c_str(),_port);
        #endif
            //if(networkInsecure.connect(_brokerUri.c_str(),_port))
                //Serial.println("MQTT Connected!");
    }
    client.connect(_brokerUri.c_str());
    
    
  
    return true;
}

/// @brief Durable delivery. Message will wait in queue until it is confirmed as delivered to host
void esp32_mqtt_client::loop(){
    if(_publishList.empty())
        return;
    //if there is anything to piublish, do so
     if(!isConnected())
        connect();
    #ifdef DEBUG
    Serial.printf("Processing %d publications\n", _publishList.size());
    #endif
    //int qos = 0;
    auto it = _publishList.end();
    do{
        it--;
        bool result = client.publish((*it).first.c_str(),(*it).second.c_str());
        #ifdef DEBUG
        Serial.printf("Publishing to topic %s %s. Result: %s\n", (*it).first.c_str(),(*it).second.c_str(), result ? "sucessfull" : "failed");
        #endif
        //logger.logDebug(string_format("Publishing. Topic: [%s] Value [%s] - Result: %s", (*it).first.c_str(), (*it).second.c_str(), result ? "suceeded" : "failed"));
        if(!result) return; // stop processing
        //_publishList.pop_back();

        //print address of itterator and begining
        //Serial.printf("Itterator: 0x%08X\t Beginning: 0x%08X\n", &(*it),  &(*_publishList.begin()));
    } while(it != _publishList.begin());
    _publishList.clear();
    //TODO: figure out why this prevents server from responding to requests.
    //  possibly timers are being reallocated
    if(_subscribeEnabled)
        client.loop();
}



void esp32_mqtt_client::disconnect()
{
    _port == 8883 ? networkSecure.stop() : networkInsecure.stop();
}
//TODO: This functionality is not yet implemented/verified
void esp32_mqtt_client::subscribe(const char *topic, void (*callback)(char* topic, uint8_t* payload, unsigned int size))
{
    // client.subscribe(topic);
    // client.onMessage(callback);
    logger.logDebug(string_format("Subscribed to %s\n", topic));
    client.subscribe(topic,2);
    client.setCallback(callback);
}


bool esp32_mqtt_client::publish(const char *topic, const char* data)
{
    if(!_enabled) return false;
    _publishList.push_back(pair<string, string>(topic + '\0', data + '\0')); //ask my why
    logger.logInfo(string_format("Published %s to topic %s", data, topic),esp32_log_type::device);
    #ifdef DEBUG
    Serial.printf("Added message %s to topic %s\n", data, topic);
    #endif
   
//   // prepare message
//   lwmqtt_message_t message = lwmqtt_default_message;
//   message.payload = (uint8_t *)data;
//   message.payload_len = strlen(data);
//   message.retained = false;
//   message.qos = lwmqtt_qos_t(qos);

//   // prepare options
//   lwmqtt_publish_options_t options = lwmqtt_default_publish_options;

  

//   // publish message
//   auto error = lwmqtt_publish(&lwmqtt_client, &options, lwmqtt_string(topic), message, _timeout);
//   logger.logDebug(string_format("%s publishing %s to topic %s\n", error == LWMQTT_SUCCESS ? "Suceeded" : "Failed", data, topic));
//   if (error != LWMQTT_SUCCESS) {
//     // close connection
//     disconnect();

    return true;
  //}

 // return true;
    
   //bool result = client.publish(topic,data);
   
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





//