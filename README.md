# ESP32 Secure Web Server with Editor and MVC framework

## Getting Started

1. Clone the repo
 `git clone $repo`
1. Open in VS Code with Platform IO
    - On the Platform IO extension, select open folder
1. Set your WiFi creds in esp32_wifi.h
    `//TODO: move wifi creds to config file`
1. Upload Filesystem Image

1. Upload binary/elf file

1. Monitor to see IP address assigned.

## Editor

Navigate to /edit to use the advanced system editor


## Project Structure
```
[PROJECT ROOT]
 ┣ data
 ┃ ┣ INT                            ***  Directory for internal storage, protected by middleware
 ┃ ┗ W                              ***  web directory
 ┃ ┃ ┣ CSS      
 ┃ ┃ ┃ ┗ style.css                  ***  site wide stylesheet
 ┃ ┃ ┣ IMG                          ***  put your images here
 ┃ ┃ ┃ ┣ ajax_loader.gif    
 ┃ ┃ ┃ ┗ vsizegrip.png
 ┃ ┃ ┣ JS                           ***  Javascript for site/page
 ┃ ┃ ┃ ┣ ace.js.gz
 ┃ ┃ ┃ ┣ auth.js
 ┃ ┃ ┃ ┣ edit.js
 ┃ ┃ ┃ ┣ esp32_home.js
 ...
 ┃ ┃ ┃ ┣ startup.js.gz
 ┃ ┃ ┣ T                            ***  Templates
 ┃ ┃ ┃ ┣ M                          ***  Model files in json
 ┃ ┃ ┃ ┃ ┣ _footer.json
 ┃ ┃ ┃ ┃ ┗ esp32_config_list.json
 ┃ ┃ ┃ ┣ V                          ***  View files
 ┃ ┃ ┃ ┃ ┣ _footer.html
 ┃ ┃ ┃ ┃ ┣ esp32_config_list.html
 ┃ ┃ ┃ ┃ ┣ esp32_system_info.html
 ┃ ┃ ┃ ┃ ┗ esp32_home.html
 ┃ ┃ ┃ ┣ _template.html             ***    +-------------------------------------------------+
 ┃ ┃ ┃ ┣ head.html                  ***    |   ** DO NOT REMOVE OR THINGS MIGHT BREAK **     |
 ┃ ┃ ┃ ┣ header_int.html            ***    |       ==============================            |
 ┃ ┃ ┃ ┣ header_pub.html            ***    |       Files that are part of the                |
 ┃ ┃ ┃ ┣ menu_int.html              ***    |       MVC framework.                            |
 ┃ ┃ ┃ ┣ menu_pub.html              ***    |       ==============================            |
 ┃ ┃ ┃ ┣ title_int.html             ***    |   ** DO NOT REMOVE OR THINGS MIGHT BREAK **     |
 ┃ ┃ ┃ ┗ title_pub.html             ***    +-------------------------------------------------+
 ┃ ┃ ┣ edit.html                    ***  File Editor to modify your html, css and javascript files for rapid development
 ┃ ┃ ┣ favicon.ico                  ***  Site icon
 ┃ ┃ ┣ index.html                   ***  Fallback landing page if MVC fails
 ┣ src
 ┃ ┣ System                         ***  ESP32 Secure Server System Files
 ┃ ┃ ┣ AUTH                         ***        Authentication related files
 ┃ ┃ ┃ ┣ ArduinoJWT.cpp
 ┃ ┃ ┃ ┣ ArduinoJWT.h
 ┃ ┃ ┃ ┣ base64.hpp
 ┃ ┃ ┃ ┣ cert.h
 ┃ ┃ ┃ ┣ esp32_middleware.cpp
 ┃ ┃ ┃ ┣ esp32_middleware.h
 ┃ ┃ ┃ ┣ key.h
 ┃ ┃ ┃ ┣ sha256.cpp
 ┃ ┃ ┃ ┗ sha256.h
 ┃ ┃ ┣ CORE
 ┃ ┃ ┃ ┣ base_controller.cpp        ***  Parent class from which all controllers inherit
 ┃ ┃ ┃ ┣ base_controller.hpp        ***  Parent class from which all controllers inherit
 ┃ ┃ ┃ ┣ esp32_fileio.cpp           ***  File operations
 ┃ ┃ ┃ ┣ esp32_fileio.h             ***  File operations
 ┃ ┃ ┃ ┣ esp32_server.cpp           ***  HTTP and HTTPS server host
 ┃ ┃ ┃ ┣ esp32_server.h             ***  HTTP and HTTPS server host
 ┃ ┃ ┃ ┣ esp32_wifi.cpp             ***  Wifi connectivity
 ┃ ┃ ┃ ┗ esp32_wifi.h               ***  Wifi connectivity
 ┃ ┃ ┣ MODULES
 ┃ ┃ ┣ ROUTER
 ┃ ┃ ┃ ┣ esp32_router.cpp           ***  Routes requests for MVC. Defaults to SPIFFS file server hosting
 ┃ ┃ ┃ ┣ esp32_router.h             ***  Routes requests for MVC. Defaults to SPIFFS file server hosting
 ┃ ┃ ┃ ┣ esp32_template.cpp         ***  MVC Template Engine
 ┃ ┃ ┃ ┗ esp32_template.h           ***  MVC Template Engine
 ┃ ┃ ┗ Config.h
 ┃ ┣ data
 ┃ ┃ ┗ C                            ***  Directory for your MVC controller code
 ┃ ┃ ┃ ┣ _footer_controller.cpp
 ┃ ┃ ┃ ┣ _footer_controller.hpp
 ┃ ┃ ┃ ┣ esp32_config_controller.cpp
 ┃ ┃ ┃ ┣ esp32_config_controller.hpp
 ┃ ┃ ┃ ┣ esp32_system_info_controller.cpp
 ┃ ┃ ┃ ┣ esp32_system_info_controller.hpp
 ┃ ┃ ┃ ┣ esp32_home_controller.cpp
 ┃ ┃ ┃ ┣ esp32_home_controller.hpp
 ┃ ┃ ┃ ┣ esp32_wifi_controller.cpp
 ┃ ┃ ┃ ┗ esp32_wifi_controller.hpp
```


## MVC
- Content files related to a view should be placed in CSS, IMG and JS directorries accordingly.
- Model and View files related to the MVC should be managed in `data/W/T` directory.
- Controller files are in `src/data/C`

The directory structure is as follows

```
[PROJECT ROOT]
 ┣ data
 ┃ ┗ W                              ***  web directory
 ┃ ┃ ┣ CSS                          ***  stylesheets
 ┃ ┃ ┣ IMG                          ***  images
 ┃ ┃ ┣ JS                           ***  Javascript
 ┃ ┃ ┣ T                            ***  Templates
 ┃ ┃ ┃ ┣ M                          ***  Model files in json
 ┃ ┃ ┃ ┣ V                          ***  View files
 ┣ src
 ┃ ┣ data
 ┃ ┃ ┗ C                            ***  Controller
```



### Controller
- contains one or more actions
- can interact with esp32 hardware
- set variables for the view
- requires compilation on host for changes to take effect.
- C++

### Model
- static data that can be used by the view
- can be modified using editor or otherwise by code
- JSON format
- naming convention follows `CONTROLLER_ACTION.json`
    e.g. for a controller `esp32_config` and action `list` the resulting model is `esp32_config_list.json`

### View
- html to render content
- can use variables from the model
- can use variables from the controller
- naming convention follows `CONTROLLER_ACTION.html`
    e.g. for a controller `esp32_config` and action `list` the resulting model is `esp32_config_list.html`


## MVC Structure

### Components

 1. _template.html - Layout of site
 1. head.html - html to be placed in the `<head>` of the HTML document
 1. header_int.html - header for authenticated users
 1. header_pub.html - header for public
 1. menu_int.html - menu for authenticated users
 1. menu_pub.html - menu for public 
 1. title_int.html - title for authenticated users
 1. title_pub.html - title for public

 ### Actions
 1. index - default
 1. list - list items
 1. put -  update item
 1. post - add item
 1. delete - delete item
 1. options - options for controller


 ### Imeplementation of a custom controler

    Let's create a scenario of a new controller and view we would like to create.
    Consider we want to have add a task list.
---

Controller should have the following actions:
  1. index    - to show us a detail view of a task
  2. list     - to show us a list of all tasks
  3. put      - to update a task
  4. post     - to craete a task
  5. delete   - to delete a task

Let's consider the naming conventions for our files
1. Controller: `custom_task_manager_controller.(cpp/hpp)`
  a. __`custom_task_manager`__ explains what the controller should do.
  b. __`_controller`__ suffix is a naming convention for all controllers.
2. Views: Due to SPIFFS 32 char restriction, these should be as short as meaningfully possible
  a. `c_t_m.html`
  b. `c_t_m.html`
  c. We do not need views for put, post, and delete. These methods can be handled by the controller code without requiring a view.

---

 #### Add controller

##### Create a new file in `src/Data/C` named `custom_task_manager_controller.hpp`.
```cpp
#ifndef _ESP32__CONTROLLER_CONFIG_H
#define _ESP32__CONTROLLER_CONFIG_H
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "System/CORE/base_controller.hpp"

using namespace httpsserver;
class custom_task_manager_controller : public Base_Controller {
public:
    //For each method we will implement, we specifiy the method and overwrite its isImplemented property
    inline virtual void Index(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isIndexImplemented(){ return true;}

    inline virtual void List(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isListImplemented(){ return true;}

    inline virtual void Put(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isPutImplemented(){ return true;}

    inline virtual void Post(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isPostImplemented(){ return true;}

    inline virtual void Delete(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isDeleteImplemented(){ return true;}

private:
    static DerivedController<custom_task_manager_controller> reg; //register the controller
};
#endif
```
##### Create a new file in `src/Data/C` named `custom_task_manager_controller.cpp`.

```cpp
#include "custom_task_manager_controller.hpp"


DerivedController<custom_task_manager_controller> custom_task_manager_controller::reg("c_t_m");

void custom_task_manager_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    res->printf("Serving index action for custom_task_manager\n");
}
void custom_task_manager_controller::List(HTTPRequest* req, HTTPResponse* res) {
    res->printf("Serving list action for custom_task_manager\n");
}
void custom_task_manager_controller::Put(HTTPRequest* req, HTTPResponse* res) {
    res->printf("Serving put action for custom_task_manager\n");
}
void custom_task_manager_controller::Post(HTTPRequest* req, HTTPResponse* res) {
    res->printf("Serving post action for custom_task_manager\n");
}
void custom_task_manager_controller::Delete(HTTPRequest* req, HTTPResponse* res) {
    res->printf("Serving delete action for custom_task_manager\n");
}
```
>   For now we are just adding stubs. We will implement these later

Compile and navigate to {HOST}/index.html
You should not see the new controller registered with all of its actions
Try to navigate to any action.
You will see an error like 
>Template /W/T/V/custom_task_manager.html not found! 

So let's add our templates.

 #### Add view

##### Create a new file in `data/W/T/V` named `custom_task_manager.html`.
This will be served for the index action

##### Create a new file in `data/W/T/V` named `custom_task_manager_list.html`.
This will be served for the list action

Notice the naming convetion. Filename consists of the name of the controller (without the _controller suffix)___action__

> Upload Filesystem Image from Platform IO extension and refresh the page after its done.

 
 #### Persistance
 We want to be able to store our task list to a file for persistant storage
 Files that contain data should only be stored in PRIVATE storage.
##### Create a new file in `data/INT` named `custom_task_manager.json`. 
Add the following text so we have some mock data to explore
```json
[
    {
        "name":"Get Grocieries",
        "createdBy": "Admin",
        "completed": false,
        "createdOn": "2024-04-21 16:24:33"
    },
    {
        "name":"Pay Bills",
        "createdBy": "Accountant",
        "completed": false,
        "createdOn": "2024-02-13 06:15:57"
    }
]
```

Let's add code to display the list on the controller's list action

#### Pass data to view from controller
##### Modify `custom_task_manager_controller.cpp` in  `src/Data/C` .



Change
```cpp
void custom_task_manager_controller::List(HTTPRequest* req, HTTPResponse* res) {
    res->printf("Serving index action for custom_task_manager\n");
}
```
to 
```cpp
void custom_task_manager_controller::List(HTTPRequest* req, HTTPResponse* res) {
    // Read tasks from file
    StaticJsonDocument<1024> doc;
    File taskFile = SPIFFS.open("/INT/custom_task_manager.json","r");
    if(taskFile.size() == 0)
    {
        //TODO: 
    }
    DeserializationError error = deserializeJson(doc, taskFile);
    if (error)
        Serial.println(F("Failed to read task, setting to empty list"));


    string outputString;
    serializeJson(doc, outputString);
    //set to template variable
    controllerTemplate.SetTemplateVariable("$_TASKS", outputString.c_str());
    
}
```

##### Modify `c_t_m.html` to add `$_TASKS`

Upload your filesystem and upload your elf. refresh the `/c_t_m/list` page.

You should see a JSON array representing our mock data.
Congratulations, you have read data from a file on the ESP32 SPIFFS volume and passed it to the view.

 #### Add Javascript (optional)
    Let's expand on our task list by passing the $_TASKS variable to some javascript.
    We will use javascript to add nice controls to our view
##### Modify `c_t_m.html`

```html
<style>
    html, body{
        display:flex;
        width:100%;
        height:100%;
        flex-direction:column;
    }
    #task-list-container{
        display: flex;
        flex-grow: 1;
        flex-direction: column;        
    }
    #task-list-header{
        background-color: rgb(73, 14, 78);
        border: solid 2px rgb(17, 0, 19);
        padding:4px;
        color:#fff;
    }
    #task-list, #task-list-header{
        display:grid;
        grid-template-columns: minmax(30%,50%) 140px 160px 200px;        
    }
</style>
<div id="task-list-container">
    <div id="task-list-header">
        <div>Task</div>
        <div>Completed?</div>
        <div>Created By</div>
        <div>Created On</div>
    </div>
    <div id="task-list"></div>
</div>

<script type="text/javascript" defer>
    function loadTasks(tasks){
        var taskListElement = document.getElementById('task-list');
        if(taskListElement === undefined) return;

        //var tasks = JSON.parse(taskJson);
        tasks.forEach((element) => {
            //var taskRow = document.createElement('div');
            var taskName = document.createElement('div');
            var createdBy = document.createElement('div');
            var completed = document.createElement('div');
            var createdOn = document.createElement('div');
            taskName.textContent = element['name'];
            createdBy.textContent = element['createdBy'];
            createdOn.textContent = element['createdOn'];
            completed.textContent = element['completed'] ? 'Yes' : 'No';

            taskListElement.appendChild(taskName);
            taskListElement.appendChild(completed);
            taskListElement.appendChild(createdBy);
            taskListElement.appendChild(createdOn);
        });
    }
    loadTasks($_TASKS);
</script>
```

There are three parts in our changes to the template
1. We added __CSS styles__ using
```html
<style> ... </style>
```

2. We added __JavaScript__ using 
```html
<script type="text/javascript" defer>
</script>
 ``` 
  - notice the defer attribute. this should ensure the code is called after the page is loaded

3. We added __HTML__ code, but just enought for our logic in Javascript to create what is needed.
 
 #### Move CSS to a file (optional)

 The CSS in this html file can be moved into the `/W/CSS` folder.
 It can be referenced using
 ```html
 <script type="text/javascript" src="/CSS/name_of_file.cs">
 ```

 #### Pass data to controller from view

There are 3 fundemental ways to pass data to the backend (controller) from your view.
1. You can make requests to the /api methods to execute logic. This does not directly impact your controller
2. You can pass parameters in the query string 
> e.g. c_t_m/list?showDeleted=false
3. You can make POST, PUT and DELETE requests to your controllers.

Let's look at passing parameters in the query string. For this example we will implement the following use case
> Opening the page, I would like to filter by the user who created the entry.

To accomplish this, we are going to modfiy `custom_task_manager_controller.cpp`.

Change 
```cpp
string outputString;
serializeJson(doc, outputString);
```
to
```cpp
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
```
Compile and upload. First refresh the page to make sure it works without a filter.
Then add a filter to the request string
`/c_t_m/list?createdBy=Admin`

And you should see only one record returned.
__WHAT WE LEARNED__
With this process, we can implement client side controls that interact with the backend code.
Think search box, paginator, etc.


#### Pass Data to backend using POST

> Easy
   - In this example, we will add the ability to toggle the completed state of the note
   We will need to 
     - add a button on each record that can post its information with an action to toggle complete status
     - refresh the task list

     modify `c_t_m.html` to add the javascript we need to post the request and wire up the click event

```javascript
    <script type="text/javascript" defer>
    function httpPostProcessRequest() {
        if (j.readyState == 4) {
            if (j.status != 200)
                alert("ERROR[" + j.status + "]: " + j.responseText)            
            else loadTasks(JSON.parse(j.responseText));
        }
    }
    function httpPost(taskName, data) {
        j = new XMLHttpRequest();
        j.onreadystatechange = httpPostProcessRequest;
        var d = new FormData();
        d.append("data", new Blob([taskName], {data}));
        j.open("POST", "/c_t_m?ToggleCompleted/" + taskName);
        j.send(d)
    }

    function toggleCompletedStatus(taskName, isCompleted){
        httpPost(taskName,isCompleted ? false : true);
    }
    function loadTasks(tasks){
        var taskListElement = document.getElementById('task-list');
        taskListElement.innerHTML = '';
        if(taskListElement === undefined) return;

        //var tasks = JSON.parse(taskJson);
        tasks.forEach((element) => {
            //var taskRow = document.createElement('div');
            var taskName = document.createElement('div');
            var createdBy = document.createElement('div');
            var completed = document.createElement('div');
            var createdOn = document.createElement('div');
            taskName.textContent = element['name'];
            createdBy.textContent = element['createdBy'];
            createdOn.textContent = element['createdOn'];
            completed.textContent = element['completed'] ? 'Yes' : 'No';
            completed.addEventListener('click', function(event) {
                toggleCompletedStatus(taskName.textContent, element['completed']);            
            });

            taskListElement.appendChild(taskName);
            taskListElement.appendChild(completed);
            taskListElement.appendChild(createdBy);
            taskListElement.appendChild(createdOn);
        });
    }
    loadTasks($_TASKS);
</script>
```
We will take the reponse and use our loadTasks method to reload the tasks

On the backend, let's imlpement the post method

```cpp
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
```
We check if the first part of the path matches our intended post action `ToggleCompleted`.
If it does, we find the item. If found, we update it. We return the list, updated or not.

> Medium

In this example, we will add the ability to add a note.
We will need to implement the index controller for a detailed view.
  - The view should show empty form fields for a new record
  - The view should show form fields loaded with data on the selected object
  - The veiw should expose a function to submit the form
  - The controller should accept post HTTP method and create a new entry in our persisted task list



### Defining custom actions
...

## Troubleshooting

### Forgetting to add stuff

#### Missing template (html) file.
    ![Missing Template](documentation/images/issues-missing-tempalte)