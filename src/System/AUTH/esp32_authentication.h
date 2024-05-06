
#include "string"
#include "System/Config.h"
#include <SPIFFS.h>
#include "ArduinoJson.h"
using namespace std;
#define PASSWORD_ENCRYPTION_SECRET "CHANGE_ME_RIGHT_AWAY!!!11"

struct esp32_user_auth_info{
    string username = "";
    string password = "";
    string role = "";
    bool enabled;
    bool authenticated = false;
};

enum ChangePasswordResult{
    WrongPassword = 0,
    BadPasswordFormat = 1,
    SamePassword = 2,
    Ok = 3,
    AuthSystemError = 4,
};

class esp32_authentication{

    public:

    static esp32_user_auth_info authenticateUser(const char* username, const char* password);
    static bool registerUser(const char* username, const char* password, const char* role);
    static ChangePasswordResult changePassword(const char* username, const char* oldPassword, const char* newPassword);

    static JsonObject findUser(JsonArray users, const char* userName);

    static bool encodePassword(const char * plainPassword, string & output);
    //static bool decodePassword(const char * encodedPassword, string & output);
    static bool verifyPassword(const char* username,const char* password);
    private:

    

    //static JsonVariant findNestedKey(JsonObject obj, const char* key) ;
    
};