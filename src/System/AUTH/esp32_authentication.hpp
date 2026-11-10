
#include "string"
#include "System/Config.h"
#include "System/AUTH/esp32_sha256.h"
#include "ArduinoJson.h"
#include "esp32_filesystem.hpp"
using namespace std;
extern esp32_file_system filesystem;
struct esp32_user_auth_info{
    string username = "";
    string sessionId = ""; // per-user random id embedded in the JWT; rotated on password change
    string role = "";
    bool enabled;
    bool authenticated = false;
    bool setupRequired = false; // no users exist yet; the first login needs the setup code
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

    // Verify a plain-text password (login). Never accepts a password hash.
    static esp32_user_auth_info authenticateUser(const char* username, const char* password, const char* setupCode = nullptr);
    // True until the first user is created.
    static bool setupRequired();
    // Prints the setup code to the serial console while setupRequired().
    static void announceSetupCode();
    // Validate a session id taken from a signed JWT. Cheap; used on every request.
    static esp32_user_auth_info validateSession(const char* username, const char* sessionId);
    // Log out: rotates the user's session id if it matches, so every token carrying it stops working.
    static bool revokeSession(const char* username, const char* sessionId);
    static bool registerUser(const char* username, const char* password, const char* role, bool enabled = true);
    static ChangePasswordResult changePassword(const char* username, const char* oldPassword, const char* newPassword);
    //helper methods
    static JsonObject findUser(JsonArray users, const char* userName);

    
    static bool verifyPassword(const char* username,const char* password);
    
    
    private:
    static string _setupCode;
    static const char* setupCode();
    // Produces "$p2$<iterations>$<salt hex>$<hash hex>" (PBKDF2-HMAC-SHA256, random salt)
    static string hashPassword(const char *plainPassword);
    // Checks a plain password against a stored value (PBKDF2 format or legacy unsalted SHA-256 hex).
    static bool checkStoredPassword(const char *plainPassword, const char *stored, bool &needsUpgrade);
    static string newSessionId();
    static bool loadUsers(DynamicJsonDocument &doc);
    static bool saveUsers(DynamicJsonDocument &doc);
    static size_t authDocCapacity();

    static void legacySha256(const char *plainPassword, byte * output);
    static void binaryPasswordToString(byte* encryptedPass,char* storedPass, size_t length = SHA256_SIZE);
    static bool stringPasswordToBinary(const char * storedPass, byte* encryptedPass, size_t length = SHA256_SIZE);
    static bool constantTimeEquals(const byte* a, const byte* b, size_t length);
    
    //static uint8_t *_hash;
};