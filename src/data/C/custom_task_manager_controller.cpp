#include "custom_task_manager_controller.hpp"
#define TASK_LIST_FILE_NAME "/INT/custom_task_manager.json"

DerivedController<custom_task_manager_controller> custom_task_manager_controller::reg("c_t_m");

void custom_task_manager_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    res->printf("Serving index action for custom_task_manager\n");
}
void custom_task_manager_controller::List(HTTPRequest* req, HTTPResponse* res) {
    // Read tasks from file
    StaticJsonDocument<512> doc;
    File taskFile = SPIFFS.open(TASK_LIST_FILE_NAME,"r");
    if(taskFile.size() == 0)
    {
        //TODO: 
    }
    DeserializationError error = deserializeJson(doc, taskFile);
    if (error)
        Serial.println(F("Failed to read task, setting to empty list"));
    taskFile.close();

    string outputString;
    string createdBy;
    if(req->getParams()->getQueryParameter("createdBy",createdBy)){
        String createdByFilter = String(createdBy.c_str());
        StaticJsonDocument<512>filtered;
        JsonArray inputTasks = doc.as<JsonArray>();
        
        for(auto task : inputTasks){
            String createdByRecord = String(task["createdBy"].as<const char *>());
            if(createdByRecord.equalsIgnoreCase(createdByFilter)){
                filtered.add(task.as<JsonObject>());
            }
        }
        serializeJson(filtered, outputString);    
    } else
        serializeJson(doc, outputString);
    //set to template variable
    controllerTemplate.SetTemplateVariable("$_TASKS", outputString.c_str());
    
}
void custom_task_manager_controller::Put(HTTPRequest* req, HTTPResponse* res) {
    res->printf("Serving put action for custom_task_manager\n");
}
void custom_task_manager_controller::Post(HTTPRequest* req, HTTPResponse* res) {
    String path = String(route.params.c_str());
    int startIdx = path.indexOf("/");
    String action = path.substring(0,startIdx);
    string outputString;
    
    if (startIdx > 0) {
        if(strcmp(action.c_str(),"ToggleCompleted") == 0){
            String value = path.substring(startIdx + 1);
            //res->printf("Action %s with value %s received\n", action.c_str(), value.c_str());
            String taskNameFilter = String(value.c_str());

            StaticJsonDocument<512> doc;
            File taskFile = SPIFFS.open(TASK_LIST_FILE_NAME,"r");
            if(taskFile.size() == 0)
            {
                //TODO: 
            }
            DeserializationError error = deserializeJson(doc, taskFile);
            taskFile.close();
            if (error){
                res->printf("Failed to read task list file");
                return;
            }

            
            JsonArray inputTasks = doc.as<JsonArray>();
            
            for(auto task : inputTasks){
                String taskName = String(task["name"].as<const char *>());
                Serial.printf("Comparing task %s to %s", taskName.c_str(), value.c_str());
                if(taskName.equalsIgnoreCase(value)){
                    bool isCompleted = task["completed"].as<bool>();
                    task["completed"] = !isCompleted;
                    Serial.printf("Setting task %s to %s", taskName.c_str(), isCompleted ? "Completed" : "Not Completed");
                    break;
                }
            }
            taskFile = SPIFFS.open(TASK_LIST_FILE_NAME,"w");
            serializeJson(doc, taskFile);            
            taskFile.close();

            
            serializeJson(doc, outputString);            
            res->print(outputString.c_str());
            
        }
        else{
            res->printf("Unknown action %s\n", action.c_str());
        }
    }    
}
void custom_task_manager_controller::Delete(HTTPRequest* req, HTTPResponse* res) {
    res->printf("Serving delete action for custom_task_manager\n");
}
