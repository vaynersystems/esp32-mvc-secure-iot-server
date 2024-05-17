
#ifndef _ESP32_BASE_SERVICE_H
#define _ESP32_BASE_SERVICE_H

#include <map>
#include <vector>
#include "../Config.h"
#include <ArduinoJson.h>

#include "string_extensions.h"
#include "../ROUTER/esp32_routing.h"

#include <algorithm>


class esp32_base_service {
    public:
        esp32_base_service() {};
        ~esp32_base_service() {};

        
        /// @brief Execute service action. Service will use parameters from route
        virtual string Execute(){ return string_format("Service %s's Execute method not implemented", route.service.c_str());}

        void SetRoute(esp32_service_route reqRoute) {
            route = esp32_service_route(reqRoute);
        }
    
protected:
    esp32_service_route route;   

};

template<typename T> esp32_base_service* createT() { return new T; }

struct BaseServiceFactory {
public:
    typedef std::map<std::string, esp32_base_service * (*)()> map_type;


public:

    static esp32_base_service* createInstance(esp32_service_route reqRoute) {
        map_type::iterator it = getMap()->begin();
        while (it != getMap()->end()) {
            if (it->first == reqRoute.service.c_str()) { //set view
                auto ctrl = it->second();
                ctrl->SetRoute(reqRoute);         
                return ctrl;
            }
            it++;
        }
        return 0;       
    }
    static esp32_base_service* createInstance(std::string service, std::string query = "")
    {
        esp32_service_route route;
        route.service = service;
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
    static std::pair<std::string, esp32_base_service*(*)()> getInstanceAt(int idx) {
        map_type::iterator it = getMap()->begin();
       
        for (int i = 0; i < idx && i < getMap()->size(); i++) it++;
        return *it;
    }
    /// @brief Chjecks if instance of service has been registered
    /// @param s Service name
    /// @return Index of service. -1 if not found. 
    static uint8_t indexOf(std::string const& s) {
        if(getInstanceCount() <= 0 ) return -1;
        map_type::iterator it = getMap()->begin();
        uint8_t idx = 0;
        while (it != getMap()->end()) {
            if (it->first == s) return idx;            
            it++; idx++;
        }        
        //HTTPS_LOGW("Service instance of %s not found among the %i controllers\n",s.c_str(), getInstanceCount());
        return -1;
       
    }

protected:
    static map_type* serviceMap;
    
    static map_type* getMap() {
        // never delete'ed. (exist until program termination)
        // because we can't guarantee correct destruction order 
        if (!serviceMap) { serviceMap = new map_type; }
        return serviceMap;
    }

   
};

template<typename T>
struct DerivedService : BaseServiceFactory {
public:
    DerivedService(std::string const& s) {
        auto t = &createT<T>;
        getMap()->insert(std::make_pair(s, t ));
        _name = s.c_str();
    }  
    ~DerivedService(){
        getMap()->erase(_name.c_str());
    }  
private:
    string _name;
};
#include "../ROUTER/esp32_router.h"


#endif