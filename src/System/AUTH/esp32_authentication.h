
#include "string"
#include "System/Config.h"
#include <SPIFFS.h>
#include "ArduinoJson.h"
using namespace std;

struct esp32_user_auth_info{
    string username = "";
    string password = "";
    string role = "";
    bool enabled;
    bool authenticated = false;
};

class esp32_authentication{

    public:

    static esp32_user_auth_info authenticateUser(const char* username, const char* password);
    static bool registerUser(const char* username, const char* password, const char* role);
    static bool changePassword(const char* username, const char* oldPassword, const char* newPassword);

    private:

    //static JsonVariant findNestedKey(JsonObject obj, const char* key) ;
    static JsonObject findUser(JsonArray users, const char* userName);
};