#include "esp32_config_controller.hpp"
#include "string_extensions.h"
#include <WiFi.h>

DerivedController<esp32_config_controller> esp32_config_controller::reg("esp32_config");

void esp32_config_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    
    title = "Module Configuration Page";

    

    controllerTemplate.SetTemplateVariable("$_CUSTOM_MESSAGE", "<B>[Dynamic - Template Driven Data]</B>");

    int paramCount = req->getParams()->getQueryParameterCount();
    res->printf("<p class='debug-small'>Page from ip [%s] with [%u]parameters\n", req->getClientIP().toString().c_str(), paramCount);
    if (paramCount > 0) {
        auto ittr = req->getParams()->beginQueryParameters();
        for (int i = 0; i < paramCount; i++) {
            res->printf("\tParam [%s]: [%s]\n", ittr->first.c_str(), ittr->second.c_str());
            ittr++;
        }
    }
    res->printf("</p>");
}

/// @brief Overwrite Action since we have custom action implemented
/// @param req 
/// @param res 
void esp32_config_controller::Action(HTTPRequest* req, HTTPResponse* res) {
    if (route.action.compare("GetAvailableWifi") == 0) {
        GetAvailableWifi(req,res);
    }
    else
        Base_Controller::Action(req,res);
}


//custom action to get wifi list, or other list
void esp32_config_controller::GetAvailableWifi(HTTPRequest* req, HTTPResponse* res) {
     int n = WiFi.scanNetworks();
     StaticJsonDocument<1024> doc;
     JsonArray networks = doc.to<JsonArray>();

     for (int i = 0; i < n; ++i) {
        auto network = networks.createNestedObject();
        string encryption;
        switch (WiFi.encryptionType(i))
        {
            case WIFI_AUTH_OPEN:
                encryption = "open";
                break;
            case WIFI_AUTH_WEP:
                encryption = "WEP";
                break;
            case WIFI_AUTH_WPA_PSK:
                encryption = "WPA";
                break;
            case WIFI_AUTH_WPA2_PSK:
                encryption = "WPA2";
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                encryption = "WPA+WPA2";
                break;
            case WIFI_AUTH_WPA2_ENTERPRISE:
                encryption = "WPA2-EAP";
                break;
            case WIFI_AUTH_WPA3_PSK:
                encryption = "WPA3";
                break;
            case WIFI_AUTH_WPA2_WPA3_PSK:
                encryption = "WPA2+WPA3";
                break;
            case WIFI_AUTH_WAPI_PSK:
                encryption = "WAPI";
                break;
            default:
                encryption = "unknown";
        }
        network["ssid"]= WiFi.SSID(i);
        network["rssi"]= WiFi.RSSI(i);
        network["channel"]= WiFi.channel(i);
        network["encryption"]= encryption;   
    }
    String outputstring;
    serializeJson(doc,outputstring);   
    res->print(outputstring.c_str());    
}