
#include "string"
#include "System/Config.h"
#include "System/AUTH/esp32_sha256.h"
#include "ArduinoJson.h"
#include "esp32_filesystem.hpp"
using namespace std;
extern esp32_file_system filesystem;
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
    static bool registerUser(const char* username, const char* password, const char* role, bool enabled = true);
    static ChangePasswordResult changePassword(const char* username, const char* oldPassword, const char* newPassword);
    //helper methods
    static JsonObject findUser(JsonArray users, const char* userName);

    
    //static bool decodePassword(const char * encodedPassword, string & output);
    static bool verifyPassword(const char* username,const char* password);
    
    
    private:
    static void encryptPassword(const char *plainPassword, byte * output);

    static void binaryPasswordToString(byte* encryptedPass,char* storedPass);
    static void stringPasswordToBinary(const char * storedPass, byte* encryptedPass);
    
    //static uint8_t *_hash;
};