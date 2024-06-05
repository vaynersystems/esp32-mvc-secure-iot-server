
#ifndef _ESP32_BASE_CONTROLLER_H
#define _ESP32_BASE_CONTROLLER_H

#include <map>
#include <vector>
#include "../Config.h"
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "../ROUTER/esp32_template.h"
#include <ArduinoJson.h>
#include "../ROUTER/esp32_routing.h"

#include "string_helper.h"

#include <algorithm>
using namespace httpsserver;

class esp32_base_controller {
    public:
        esp32_base_controller() {};
        ~esp32_base_controller() {            
        };

        esp32_template controllerTemplate = esp32_template();
        virtual void GenericIndex(HTTPRequest* req, HTTPResponse* res);

        virtual void Index(HTTPRequest* req, HTTPResponse* res){
            GenericIndex(req,res); //if not overwritten, build page using MVC framework
        }
        virtual void List(HTTPRequest* req, HTTPResponse* res) {
            GenericIndex(req,res); //if not overwritten, build page using MVC framework
         }
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

        
        /// @brief Function that handles controller actions. Overwrite this function in a controller to implement custom actions
        /// @param req 
        /// @param res 
        virtual void Action(HTTPRequest* req, HTTPResponse* res) {
            //set temlate variables from json file
            LoadModel();
            Serial.printf("Request for action %s on controller %s\n", route.action.c_str(), route.controller.c_str());
            if (iequals(route.action.c_str(), "LIST", 4)) {
                List(req, res);
            } else if (iequals(route.action.c_str(), "PUT", 3)) {
                Put(req, res);
            }
            else if (iequals(route.action.c_str(), "POST", 4)) {
                Post(req, res);
            }
            else if (iequals(route.action.c_str(), "DELETE", 6)) {
                Delete(req, res);
            }
            else if (iequals(route.action.c_str(), "OPTIONS", 7)) {
                Options(req, res);
            }
            else// (action == "Index") {
            {
                Index(req, res);
            }
        }
        
        virtual void GetActions(vector<string> *actions){
            if((*this).isIndexImplemented()) actions->push_back("index");
            if((*this).isListImplemented()) actions->push_back("list");
            if((*this).isPutImplemented()) actions->push_back("put");
            if((*this).isPostImplemented()) actions->push_back("post");
            if((*this).isDeleteImplemented()) actions->push_back("delete");
            if((*this).isOptionsImplemented()) actions->push_back("options");
        }

        virtual bool HasAction(const char * action){
            if(iequals(action, "index", 5)) return this->isIndexImplemented();
            if(iequals(action, "list",4)) return this->isListImplemented();
            if(iequals(action, "put",3)) return this->isPutImplemented();
            if(iequals(action, "post",4)) return this->isPostImplemented();
            if(iequals(action, "delete",6)) return this->isDeleteImplemented();
            if(iequals(action, "options",7)) return this->isOptionsImplemented();
            return false;
        }


        void SetRoute(esp32_controller_route reqRoute) {
            route = esp32_controller_route(reqRoute);
        }
        void SetTemplate(std::string customPath = std::string()) {
            controllerTemplate.templateContentFilePath = PATH_SITE_ROOT;
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
                Serial.printf("Set template content path to        %s\n", controllerTemplate.templateContentFilePath.c_str());
            }
        }

        void LoadModel() {
            std::string jsonPath = controllerTemplate.templateContentFilePath;
            jsonPath.erase(jsonPath.length() - 5);
            jsonPath.append(".json");
            int idx = jsonPath.find_last_of('/') - 1;
            jsonPath.at(idx) = 'M';//replace V with M (view with model)
            if (!SPIFFS.exists(jsonPath.c_str())) return;//nothing to do
            File dataFile = SPIFFS.open(jsonPath.c_str());
            DynamicJsonDocument doc(dataFile.size() * 2);
            DeserializationError err = deserializeJson(doc, dataFile);
            dataFile.close();
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

        

        
    string title="";
    string head="";
    string menu="";
    string footer="";    
    
protected:
    esp32_controller_route route;   

};

template<typename T> esp32_base_controller* createT() { return new T; }

struct BaseControllerFactory {
public:
    typedef std::map<std::string, esp32_base_controller * (*)()> map_type;


public:

    static esp32_base_controller* createInstance(esp32_controller_route reqRoute) {
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
    static esp32_base_controller* createInstance(std::string controller, std::string action, std::string query = "")
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
    static std::pair<std::string, esp32_base_controller*(*)()> getInstanceAt(int idx) {
        map_type::iterator it = getMap()->begin();
       
        for (int i = 0; i < idx && i < getMap()->size(); i++) it++;
        return *it;
    }
    static bool hasInstance(std::string const& s) {
        map_type::iterator it = getMap()->begin();
        while (it != getMap()->end()) {
            if (it->first == s) return true;            
            it++;
        }        
        //HTTPS_LOGW("Controller instance of %s not found among the %i controllers\n",s.c_str(), getInstanceCount());
        return false;
       
    }

protected:
    static map_type* controllerMap;
    
    static map_type* getMap() {
        // never delete'ed. (exist until program termination)
        // because we can't guarantee correct destruction order 
        if (!controllerMap) { controllerMap = new map_type; }
        return controllerMap;
    }

   
};

template<typename T>
struct DerivedController : BaseControllerFactory {
public:
    DerivedController(std::string const& s) {
        auto t = &createT<T>;
        getMap()->insert(std::make_pair(s, t ));
        _name = s.c_str();
    }  
    ~DerivedController(){
        getMap()->erase(_name.c_str());
    }  
private:
    string _name;
};
#include "../ROUTER/esp32_router.h"


#endif