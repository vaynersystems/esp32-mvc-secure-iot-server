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


## MVC
All files related to the MVC all you'd like to build should be managed in `data/W/T` directory.
The directory structure is as follows
> W - Web
>> T - Templates
>>> C - Controller    
>>> M - Model
>>> V - View

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

