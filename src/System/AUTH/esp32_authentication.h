
#include "string"
#include "System/Config.h"
#include <SPIFFS.h>
#include "ArduinoJson.h"
#include "crypto.h"
using namespace std;
// TODO; instead use NVS. at boot read if value "pwd_encrypt_key" exists. If not, create it. 
// That will persist for the lifetime of the controller or until some braniac messes with the nvs partition.
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
    //helper methods
    static JsonObject findUser(JsonArray users, const char* userName);

    
    //static bool decodePassword(const char * encodedPassword, string & output);
    static bool verifyPassword(const char* username,const char* password);
    private:
    static bool encryptPassword(const char *plainPassword, byte * output);
};