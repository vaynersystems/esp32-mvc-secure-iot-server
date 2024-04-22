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
 ┃ ┃ ┃ ┃ ┣ esp32_dashboard.html
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
 ┃ ┃ ┣ login.html                   ***  Needs to be reworked into a controller.
 ┃ ┃ ┗ logout.html                  ***  Needs to be reworked into a controller.
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
 ┃ ┃ ┃ ┣ esp32_dashboard_controller.cpp
 ┃ ┃ ┃ ┣ esp32_dashboard_controller.hpp
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
2. Views: 
  a. `custom_task_manager.html`
  b. `custom_task_manager_list.html`
  c. We do not need views for put, post, and delete. These methods can be handled by the controller code without requiring a view.

---

 #### Add controller

Create a new file in `src/Data/C` named `custom_task_manager_controller.hpp`.
```cpp

```
Create a new file in `src/Data/C` named `custom_task_manager_controller.cpp`.
```
```

 #### Add view

 #### Add Javascript (optional)
 
 #### Add CSS (optional)

 #### Pass data to view from controller

 #### Pass data to controller from view



## Troubleshooting

### Forgetting to add stuff

#### Missing template (html) file.
    ![Missing Template](documentation/images/issues-missing-tempalte)