#ifndef _ESP32_AUTHENTICATION_H
#define _ESP32_AUTHENTICATION_H

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

// Password length limits, applied by the login page, the user forms and the server alike.
// The login request only accepts PASSWORD_MAX_LENGTH characters, so a longer password could be
// saved but never used to log in.
#define PASSWORD_MIN_LENGTH 8
#define PASSWORD_MAX_LENGTH 32

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
    // A user changing their own password: the current password must be given.
    static ChangePasswordResult changePassword(const char* username, const char* oldPassword, const char* newPassword);
    // An administrator setting a user's password: no current password needed.
    static ChangePasswordResult setPassword(const char* username, const char* newPassword);
    // Gives every user in the list a new session id, so no token issued before stays valid.
    static void assignNewSessionIds(JsonArray users);
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
#endif
