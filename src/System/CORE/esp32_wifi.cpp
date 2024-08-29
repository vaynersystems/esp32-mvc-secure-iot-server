#include "esp32_wifi.h"
#include "string_helper.h"

// extern esp32_wifi wifi;
// void IRAM_ATTR diableWifi(){
//     wifi.end();    
// }

esp32_wifi::esp32_wifi(){
    
}

bool esp32_wifi::start(){
    Serial.println("Setting up WiFi");

    IPAddress addr = IPAddress(10, 0, 1, 1);
    StaticJsonDocument<512> wifiConfig;
    esp32_config::getConfigSection("wifi", &wifiConfig);
    StaticJsonDocument<768> systemConfig;
    esp32_config::getConfigSection("system", &systemConfig);
    StaticJsonDocument<256> serverConfig;
    esp32_config::getConfigSection("server", &serverConfig);


    bool isAP = wifiConfig["mode"].isNull() ? true : strcmp(wifiConfig["mode"].as<const char *>(), "access-point") == 0;

    string ntpServer = systemConfig["ntp"]["server"].as<string>();
    string timeZone = systemConfig["ntp"]["timezone"].as<string>();    
    
    string network = wifiConfig["network"].as<string>();
    string password = wifiConfig["password"].as<string>();
    

    //if wifi settings not empty and not configured for access point
    bool trySTA = !network.empty() && !password.empty() && !isAP;
    string hostname = systemConfig["hostname"].as<string>();
    bool enableMDNS = systemConfig["enableMDNS"].as<bool>();

    if(serverConfig["disableWifiAfter"].is<int>()){
        // //set up timer to disable wifi
        // timerWifiDisable = timerBegin(2,8000,true); // 48,000 / 80MHz = 0.0006
        // timerAttachInterrupt(timerWifiDisable, &diableWifi, true);
        // timerAlarmWrite(timerWifiDisable, 100000 * 1, true); // 0.0006 x 100,000 = 60 seconds
        // timerAlarmEnable(timerWifiDisable);
        // //set timer to number of minutes specified in config
    }
    
    if(trySTA){
        #ifdef DEBUG
        Serial.printf("Connecting in STA mode to network %S\n", network.c_str());        
        #endif
        bool isDHCP =  wifiConfig["dhcp"].as<bool>();
        
        
        if(!isDHCP){
            string ip = wifiConfig["ip"].as<string>();
            string subnet = wifiConfig["subnet"].as<string>();
            string gateway = wifiConfig["gateway"].as<string>();
            string dns = wifiConfig["dns"].as<string>();

            IPAddress ipAddress, subnetAddress, gatewayAddress, dnsAddress;
            if(!ipAddress.fromString(ip.c_str()) )
                Serial.println("Failed to parse ip address from config file");
            if(!subnetAddress.fromString(subnet.c_str()) )
                Serial.println("Failed to parse subnet address from config file");
            if(!gatewayAddress.fromString(gateway.c_str()) )
                Serial.println("Failed to parse gateway address from config file");
            if(!dnsAddress.fromString(dns.c_str()) )
                Serial.println("Failed to parse dns address from config file");
            
            Serial.println("Configuring custom IP address");            
            WiFi.config(ipAddress, gatewayAddress, subnetAddress, dnsAddress);
        }
    
        WiFi.begin(network.c_str(), password.c_str());
        
        unsigned long startConnectTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            Serial.print(".");
            delay(500);
            if (millis() - startConnectTime > 15000)
            {
                Serial.println("Failed to connect to wifi.");
                WiFi.disconnect(true,true);
                break;
            }
        }        
    }

    if(WiFi.status() != WL_CONNECTED){
        //failed to start in STA mode. Start in AP mode
        bool customIP = true;
        string ip = wifiConfig["ap"]["ip"].as<string>();
        string subnet = wifiConfig["ap"]["subnet"].as<string>();
        string apName = wifiConfig["ap"]["name"].isNull() ? string_format("ESP32Host_%d", (byte)esp_random()) : wifiConfig["ap"]["name"].as<string>();
        string apPassword = wifiConfig["ap"]["password"].as<string>();

        IPAddress ipAddress, subnetAddress;
        if(!ipAddress.fromString(ip.c_str()) ){
            #ifdef DEBUG
            Serial.println("Failed to parse ip address from config file");
            #endif
            customIP = false;
        }
        if(!subnetAddress.fromString(subnet.c_str()) ){
            #ifdef DEBUG
            Serial.println("Failed to parse subnet address from config file");
            #endif
            customIP = false;
        }       

        if(customIP)
            WiFi.config(ipAddress, ipAddress,subnetAddress, addr, addr);
        if( apPassword.length() <=8 )  {
            apPassword = apName.c_str(); //if no password, password matches wifi name
            #ifdef DEBUG
            Serial.printf("Setting password to %s\n", apPassword.c_str());
            #endif
        }
            WiFi.setMinSecurity(wifi_auth_mode_t::WIFI_AUTH_WEP);

        WiFi.softAP(apName.c_str(), apPassword.c_str(), 7, 0, 3);
        #ifdef DEBUG
        Serial.println("Started Wifi in AP mode");
        #endif
    }

    if(hostname.length() > 0){
        #ifdef DEBUG
        Serial.printf("Setting hostname to %s\n", hostname.c_str());
        #endif
        WiFi.setHostname(hostname.c_str());
        if(enableMDNS)
            MDNS.begin(hostname.c_str());        
    }
    
    //#ifdef DEBUG
    Serial.print("Connected. IP=");
    Serial.println(WiFi.getMode() == wifi_mode_t::WIFI_MODE_AP ? WiFi.broadcastIP() :  WiFi.localIP());
    //#endif

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, ntpServer.empty() ? "pool.ntp.org" : ntpServer.c_str());
    sntp_init();    
    
    
    if(WiFi.getMode() == WIFI_MODE_STA){
        setenv("TZ", timeZone.c_str(), 1);
        tzset();
        #ifdef DEBUG
        Serial.printf("Initializing NTP server %s\n", ntpServer.empty() ? "pool.ntp.org" : ntpServer.c_str());
        Serial.printf("Waiting for NTP time..\n");
        #endif
        struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
        do{
            gettimeofday(&tv, NULL);
            #ifdef DEBUG
            if(tv.tv_usec%500==0) Serial.print(".");
            #endif
            
        } while(tv.tv_sec < 15); //15 seconds to init NTP
    }
    //#ifdef DEBUG
    Serial.println(" done!");
    //#endif

    return WiFi.status() == WL_CONNECTED;
}

bool esp32_wifi::end(){
    #ifdef DEBUG
    Serial.println("Disabling Wifi...");
    #endif
    if(WiFi.getMode() == WiFiMode_t::WIFI_MODE_STA)
        WiFi.disconnect(true,true);
    else if(WiFi.getMode() == WiFiMode_t::WIFI_MODE_AP)
        WiFi.disconnect(true,true);
    MDNS.end();
    // timerDetachInterrupt(timerWifiDisable);
    // timerEnd(timerWifiDisable);
    return !WiFi.isConnected();
}