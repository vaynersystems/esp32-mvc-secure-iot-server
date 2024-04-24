#include "esp32_login_controller.hpp"
#define TASK_LIST_FILE_NAME "/INT/custom_task_manager.json"

DerivedController<esp32_login_controller> esp32_login_controller::reg("login");

void esp32_login_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    //just serve the login page
    //res->printf("Serving index action for esp32_login_controller\n");
}

// /// @brief Login request
// /// @param req Request object
// /// @param res Response object
// void esp32_login_controller::Post(HTTPRequest* req, HTTPResponse* res) {
//     // get username and password from form input
//     // verify against internal "database"
//     // issue token if valid.
//     // 401 if not
//     //we post to /login for middleware to take care of the work for us
    
// }
