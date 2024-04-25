
#ifndef _ESP32_BASE_CONTROLLER_H
#define _ESP32_BASE_CONTROLLER_H

#include <map>
#include <vector>
#include "../Config.h"
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "../ROUTER/esp32_template.h"
#include <ArduinoJson.h>

#include <algorithm>
using namespace httpsserver;

class Base_Controller {
    public:
        Base_Controller() {};

        esp32_template controllerTemplate = esp32_template();

        virtual void Index(HTTPRequest* req, HTTPResponse* res) {  /*res->println("This action has not been implemented");*/ }
        virtual void List(HTTPRequest* req, HTTPResponse* res) { }
        virtual void Put(HTTPRequest* req, HTTPResponse* res) { }
        virtual void Post(HTTPRequest* req, HTTPResponse* res) { }
        virtual void Delete(HTTPRequest* req, HTTPResponse* res) { }
        virtual void Options(HTTPRequest* req, HTTPResponse* res) { }

        virtual bool isIndexImplemented(){ return false;}
        virtual bool isListImplemented(){ return false;}
        virtual bool isPutImplemented(){ return false;}
        virtual bool isPostImplemented(){ return false;}
        virtual bool isDeleteImplemented(){ return false;}
        virtual bool isOptionsImplemented(){ return false;}

        
        void SetVariablesFromJSON() {
            std::string jsonPath = controllerTemplate.templateContentFilePath;
            jsonPath.erase(jsonPath.length() - 5);
            jsonPath.append(".json");
            int idx = jsonPath.find_last_of('/') - 1;
            jsonPath.at(idx) = 'M';//replace V with M (view with model)
            if (!SPIFFS.exists(jsonPath.c_str())) return;//nothing to do
            File dataFile = SPIFFS.open(jsonPath.c_str());
            DynamicJsonDocument doc(dataFile.size() * 2);
            DeserializationError err = deserializeJson(doc, dataFile);
            JsonObject configVals = doc.as<JsonObject>();

            if (err == err.Ok) {
                for (int i = 0; i < configVals.size(); i++)
                {
                    for (JsonPair kv : configVals) {
                        controllerTemplate.SetTemplateVariable(kv.key().c_str(), kv.value().as<const char*>());
                        //Serial.printf("Adding config value from json [%s:%s]\n",
                        //   kv.key().c_str(),kv.value().as<char*>());
                    }
                }
            }
        }

        /// @brief Function that handles controller actions. Overwrite this function in a controller to implement custom actions
        /// @param req 
        /// @param res 
        virtual void Action(HTTPRequest* req, HTTPResponse* res) {
            //set temlate variables from json file
            SetVariablesFromJSON();
            Serial.printf("Request for action %s on controller %s\n", route.action.c_str(), route.controller.c_str());
            if (route.action.compare("list") == 0) {
                List(req, res);
            } else if (route.action == "put") {
                Put(req, res);
            }
            else if (route.action == "post") {
                Post(req, res);
            }
            else if (route.action == "delete") {
                Delete(req, res);
            }
            else// (action == "Index") {
            {
                Index(req, res);
            }
        
        }
        void SetRoute(esp32_controller_route reqRoute) {
            route = esp32_controller_route(reqRoute);
        }
        void SetTemplate(std::string customPath = std::string()) {
            controllerTemplate.templateContentFilePath = SITE_ROOT;
            controllerTemplate.templateContentFilePath += "/T/V/"; //views folder
            if (customPath.length() > 0) {                
                controllerTemplate.templateContentFilePath += customPath;
            }
            else {
                
                controllerTemplate.templateContentFilePath += route.controller.c_str();
                if (route.action != "index") { //suffix not required for index(default) action
                    controllerTemplate.templateContentFilePath += "_";
                    controllerTemplate.templateContentFilePath += route.action.c_str();
                }
                controllerTemplate.templateContentFilePath += ".html";
            }
        }

        void GetActions(vector<string> *actions){
            if((*this).isIndexImplemented()) actions->push_back("index");
            if((*this).isListImplemented()) actions->push_back("list");
            if((*this).isPutImplemented()) actions->push_back("put");
            if((*this).isPostImplemented()) actions->push_back("post");
            if((*this).isDeleteImplemented()) actions->push_back("delete");
            if((*this).isOptionsImplemented()) actions->push_back("options");
        // }
               
        }
        bool HasAction(const char * action){
            if(strcmp(action, "index") == 0) return this->isIndexImplemented();
            if(strcmp(action, "list") == 0) return this->isListImplemented();
            if(strcmp(action, "put") == 0) return this->isPutImplemented();
            if(strcmp(action, "post") == 0) return this->isPostImplemented();
            if(strcmp(action, "delete") == 0) return this->isDeleteImplemented();
            if(strcmp(action, "options") == 0) return this->isOptionsImplemented();
            return false;
        }

        
    string title="";
    string head="";
    string menu="";
    string footer="";    
    
protected:
    esp32_controller_route route;   

};

template<typename T> Base_Controller* createT() { return new T; }

struct BaseFactory {
public:
    typedef std::map<std::string, Base_Controller * (*)()> map_type;


public:

    static Base_Controller* createInstance(esp32_controller_route reqRoute) {
        map_type::iterator it = getMap()->begin();
        while (it != getMap()->end()) {
            if (it->first == reqRoute.controller.c_str()) { //set view
                auto ctrl = it->second();
                ctrl->SetRoute(reqRoute);
                ctrl->SetTemplate();                
                return ctrl;
            }
            it++;
        }
        return 0;       
    }
    static Base_Controller* createInstance(std::string controller, std::string action, std::string query = "")
    {
        esp32_controller_route route;
        route.controller = controller;
        route.action = action;
        route.params = query;
        return createInstance(route);
    }
    static int getInstanceCount() {
        map_type::iterator it = getMap()->begin();
        int count = 0;
        for (count = 0; count < getMap()->size(); count++)
            it++;
        return count;
    }
    static std::pair<std::string, Base_Controller*(*)()> getInstanceAt(int idx) {
        map_type::iterator it = getMap()->begin();
       
        for (int i = 0; i < idx && i < getMap()->size(); i++) it++;
        return *it;
    }
    static bool hasInstance(std::string const& s) {
        map_type::iterator it = getMap()->begin();
        while (it != getMap()->end()) {
            if (it->first == s) return true;
            //Serial.printf("Controller %s does not match %s", it->first.c_str(), s.c_str());
            it++;
        }        
        //HTTPS_LOGW("Controller instance of %s not found among the %i controllers\n",s.c_str(), getInstanceCount());
        return false;
       
    }

protected:
    static map_type* map;
    
    static map_type* getMap() {
        // never delete'ed. (exist until program termination)
        // because we can't guarantee correct destruction order 
        if (!map) { map = new map_type; }
        return map;
    }

   
};

template<typename T>
struct DerivedController : BaseFactory {
public:
    DerivedController(std::string const& s) {
        auto t = &createT<T>;
        getMap()->insert(std::make_pair(s, t ));
    }    
};
#include "../ROUTER/esp32_router.h"


#endif