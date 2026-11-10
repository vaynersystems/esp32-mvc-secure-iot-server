#include "esp32_authentication.hpp"
#include "system_helper.h"
#include "base64.hpp"
#include "esp32_sha256.h"
#include "string_helper.h"
#include <esp_system.h> // esp_fill_random
#include "mbedtls/version.h"
#include "mbedtls/md.h"
#include "mbedtls/pkcs5.h"
#include <strings.h>

// PBKDF2-HMAC-SHA256 work factor. Login is the only place this runs (per-request
// authorization uses the session id), so it can be relatively slow.
#ifndef PASSWORD_PBKDF2_ITERATIONS
#define PASSWORD_PBKDF2_ITERATIONS 4096
#endif
#define PASSWORD_SALT_SIZE 16
#define SESSION_ID_SIZE 16
#define PASSWORD_HASH_PREFIX "$p2$"

#define SETUP_CODE_LENGTH 8
// No 0/O or 1/I/L, so the code is easy to read off a serial console.
static const char SETUP_CODE_ALPHABET[] = "23456789ABCDEFGHJKMNPQRSTUVWXYZ";
string esp32_authentication::_setupCode = "";

bool esp32_authentication::setupRequired(){
    return !filesystem.getDisk(SYSTEM_DRIVE)->exists(PATH_AUTH_FILE);
}

/// @brief The one-time code that must accompany the first login, which creates the administrator.
/// @brief Without it, whoever reached a new device first (its access point is open) would own it.
/// @brief Kept in RAM only and made again after every restart, so it never appears in a file or log.
const char* esp32_authentication::setupCode(){
    if(!setupRequired()){
        _setupCode.clear();
        return nullptr;
    }
    if(_setupCode.empty()){
        uint8_t random[SETUP_CODE_LENGTH];
        esp_fill_random(random, sizeof(random));
        for(size_t i = 0; i < SETUP_CODE_LENGTH; i++)
            _setupCode += SETUP_CODE_ALPHABET[random[i] % (sizeof(SETUP_CODE_ALPHABET) - 1)];
    }
    return _setupCode.c_str();
}

void esp32_authentication::announceSetupCode(){
    const char* code = setupCode();
    if(code == nullptr) return;
    Serial.printf("No users yet. Log in with a new username and password and setup code %s to create the administrator.\n", code);
}

/// @brief User authentication with a plain-text password. If the user database has not been created,
/// @brief the first login that also gives the setup code printed on the serial console becomes the system admin.
/// @brief This way the backdoor of default username/password doesn't exist.
/// @param username
/// @param password plain-text password. Password hashes are NOT accepted.
/// @param setupCode the code from announceSetupCode(); only checked while there are no users
/// @return Authentication object containing the user's information and status of request
esp32_user_auth_info esp32_authentication::authenticateUser(const char* username, const char* password, const char* setupCodeGiven){
    esp32_user_auth_info info;
    if(username == nullptr || password == nullptr) return info;

    auto drive = filesystem.getDisk(SYSTEM_DRIVE);

    // Check if the file exists
    if (!drive->exists(PATH_AUTH_FILE))
    {
        #if defined(DEBUG_SECURITY) && DEBUG_SECURITY > 0
        Serial.println("Authorization file does not exist..");
        #endif
        info.setupRequired = true;
        const char* expectedCode = setupCode();
        if(expectedCode == nullptr || setupCodeGiven == nullptr) return info;
        if(strlen(setupCodeGiven) != SETUP_CODE_LENGTH) return info;
        if(!constantTimeEquals((const byte*)expectedCode, (const byte*)setupCodeGiven, SETUP_CODE_LENGTH)) return info;
        if(strlen(username) < 3 || strlen(username) > 32 || strlen(password) == 0) return info;
        #if defined(DEBUG_SECURITY) && DEBUG_SECURITY > 0
        Serial.println("Creating");
        #endif
        //if first user, authenticate and create file
        if(!registerUser(username, password, "ADMIN")) return info;
        // fall through and authenticate against the newly created file
    }

    DynamicJsonDocument doc(authDocCapacity());
    if(!loadUsers(doc)) return info;

    JsonObject existingUser = findUser(doc.as<JsonArray>(), username);
    if(existingUser.isNull()) return info;
    if(!(existingUser["enabled"] | true)) return info;

    bool needsUpgrade = false;
    if(!checkStoredPassword(password, existingUser["password"] | "", needsUpgrade)) return info;

    bool dirty = false;
    if(needsUpgrade){ // migrate legacy unsalted SHA-256 to PBKDF2
        existingUser["password"] = hashPassword(password);
        dirty = true;
    }
    if(existingUser["sid"].isNull() || strlen(existingUser["sid"] | "") == 0){
        existingUser["sid"] = newSessionId();
        dirty = true;
    }
    if(dirty && !saveUsers(doc)) return info;

    info.username = existingUser["username"].as<const char *>();
    info.role = existingUser["role"] | "";
    info.sessionId = existingUser["sid"].as<const char *>();
    info.enabled = true;
    info.authenticated = true;
    return info;
}

/// @brief Validates the session id carried in a (signature-verified) JWT against the user database.
/// @brief Fails if the user was deleted, disabled, or changed their password since the token was issued.
esp32_user_auth_info esp32_authentication::validateSession(const char* username, const char* sessionId){
    esp32_user_auth_info info;
    if(username == nullptr || sessionId == nullptr) return info;
    if(strlen(sessionId) != SESSION_ID_SIZE * 2) return info;

    DynamicJsonDocument doc(authDocCapacity());
    if(!loadUsers(doc)) return info;

    JsonObject existingUser = findUser(doc.as<JsonArray>(), username);
    if(existingUser.isNull()) return info;
    if(!(existingUser["enabled"] | true)) return info;

    const char* storedSid = existingUser["sid"] | "";
    if(strlen(storedSid) != SESSION_ID_SIZE * 2) return info;
    if(!constantTimeEquals((const byte*)storedSid, (const byte*)sessionId, SESSION_ID_SIZE * 2)) return info;

    info.username = existingUser["username"].as<const char *>();
    info.role = existingUser["role"] | "";
    info.sessionId = storedSid;
    info.enabled = true;
    info.authenticated = true;
    return info;
}

bool esp32_authentication::revokeSession(const char* username, const char* sessionId){
    if(username == nullptr || sessionId == nullptr) return false;
    if(strlen(sessionId) != SESSION_ID_SIZE * 2) return false;

    DynamicJsonDocument doc(authDocCapacity());
    if(!loadUsers(doc)) return false;

    JsonObject existingUser = findUser(doc.as<JsonArray>(), username);
    if(existingUser.isNull()) return false;
    const char* storedSid = existingUser["sid"] | "";
    // Only the holder of the current session may end it; a stale token changes nothing.
    if(strlen(storedSid) != SESSION_ID_SIZE * 2) return false;
    if(!constantTimeEquals((const byte*)storedSid, (const byte*)sessionId, SESSION_ID_SIZE * 2)) return false;

    existingUser["sid"] = newSessionId();
    return saveUsers(doc);
}

bool esp32_authentication::registerUser(const char* username, const char* password, const char* role, bool enabled){
    if(username == nullptr || password == nullptr || role == nullptr) return false;
    if(strlen(username) == 0 || strlen(password) == 0) return false;
    if(strlen(password) > PASSWORD_MAX_LENGTH) return false;

    DynamicJsonDocument doc(authDocCapacity());
    if(!loadUsers(doc) || !doc.is<JsonArray>()){
        doc.clear();
        doc.to<JsonArray>();
    }

    if(!findUser(doc.as<JsonArray>(),username).isNull()){
        return false;
    }

    //TODO: password complexity and validation
    JsonObject newUser = doc.createNestedObject();
    newUser["username"] = username;
    newUser["password"] = hashPassword(password);
    newUser["sid"] = newSessionId();
    newUser["role"] = role;
    newUser["enabled"] = enabled;
    newUser["created"] =   getCurrentTime();

    return saveUsers(doc);
}

ChangePasswordResult esp32_authentication::changePassword(const char* username, const char* oldPassword, const char* newPassword){
    if(username == nullptr || oldPassword == nullptr || newPassword == nullptr)
        return ChangePasswordResult::BadPasswordFormat;
    if(strlen(newPassword) == 0 || strlen(newPassword) > PASSWORD_MAX_LENGTH)
        return ChangePasswordResult::BadPasswordFormat;
    if(strcmp(oldPassword,newPassword) == 0)
        return ChangePasswordResult::SamePassword;

    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    if (!drive->exists(PATH_AUTH_FILE))
        return ChangePasswordResult::AuthSystemError;

    DynamicJsonDocument doc(authDocCapacity());
    if(!loadUsers(doc))
        return ChangePasswordResult::AuthSystemError;

    JsonObject existingUser = findUser(doc.as<JsonArray>(),username);
    if(existingUser.isNull())
        return ChangePasswordResult::WrongPassword;

    bool needsUpgrade = false;
    if(!checkStoredPassword(oldPassword, existingUser["password"] | "", needsUpgrade))
        return ChangePasswordResult::WrongPassword;

    existingUser["password"] = hashPassword(newPassword);
    existingUser["sid"] = newSessionId(); // invalidate every token issued with the old password

    return saveUsers(doc) ? ChangePasswordResult::Ok : ChangePasswordResult::AuthSystemError;
}

ChangePasswordResult esp32_authentication::setPassword(const char* username, const char* newPassword){
    if(username == nullptr || newPassword == nullptr)
        return ChangePasswordResult::BadPasswordFormat;
    if(strlen(newPassword) == 0 || strlen(newPassword) > PASSWORD_MAX_LENGTH)
        return ChangePasswordResult::BadPasswordFormat;

    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    if (!drive->exists(PATH_AUTH_FILE))
        return ChangePasswordResult::AuthSystemError;

    DynamicJsonDocument doc(authDocCapacity());
    if(!loadUsers(doc))
        return ChangePasswordResult::AuthSystemError;

    JsonObject existingUser = findUser(doc.as<JsonArray>(),username);
    if(existingUser.isNull())
        return ChangePasswordResult::WrongPassword;

    existingUser["password"] = hashPassword(newPassword);
    existingUser["sid"] = newSessionId(); // log the user out everywhere

    return saveUsers(doc) ? ChangePasswordResult::Ok : ChangePasswordResult::AuthSystemError;
}

void esp32_authentication::assignNewSessionIds(JsonArray users){
    for(JsonObject user : users){
        if(user.isNull()) continue;
        user["sid"] = newSessionId();
    }
}


JsonObject esp32_authentication::findUser(JsonArray users, const char* userName){
    // Exact, case-insensitive match. (This used iequals(..., ["username"].size()), but size() is 0
    // for a JSON string, so every name matched the first user - e.g. any username logged in
    // with the first user's password.)
    if(userName == nullptr) return JsonObject();
    size_t wanted = strlen(userName);
    if(wanted == 0) return JsonObject();
    for(JsonObject seekingUser : users){
        const char* stored = seekingUser["username"] | (const char*)nullptr;
        if(stored != nullptr && strlen(stored) == wanted && strncasecmp(stored, userName, wanted) == 0)
            return seekingUser;
    }
    return JsonObject();
}


/// @brief Verify a user's plain-text password. Password hashes are NOT accepted.
bool esp32_authentication::verifyPassword(const char* username, const char* password){
    if(username == nullptr || password == nullptr) return false;
    DynamicJsonDocument doc(authDocCapacity());
    if(!loadUsers(doc)) return false;

    JsonObject existingUser = findUser(doc.as<JsonArray>(),username);
    if(existingUser.isNull()) return false;

    bool needsUpgrade = false;
    return checkStoredPassword(password, existingUser["password"] | "", needsUpgrade);
}

#pragma region Hashing

static bool pbkdf2Sha256(const char* password, const byte* salt, size_t saltLength, uint32_t iterations, byte* output, size_t outputLength){
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
    return mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,
        (const unsigned char*)password, strlen(password),
        salt, saltLength, iterations, outputLength, output) == 0;
#else
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    bool ok = mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1) == 0 &&
        mbedtls_pkcs5_pbkdf2_hmac(&ctx,
            (const unsigned char*)password, strlen(password),
            salt, saltLength, iterations, outputLength, output) == 0;
    mbedtls_md_free(&ctx);
    return ok;
#endif
}

string esp32_authentication::hashPassword(const char *plainPassword){
    byte salt[PASSWORD_SALT_SIZE];
    byte hash[SHA256_SIZE];
    char saltHex[PASSWORD_SALT_SIZE * 2 + 1];
    char hashHex[SHA256_SIZE * 2 + 1];

    esp_fill_random(salt, sizeof(salt));
    if(!pbkdf2Sha256(plainPassword, salt, sizeof(salt), PASSWORD_PBKDF2_ITERATIONS, hash, sizeof(hash)))
        return string(); // empty stored password never verifies

    binaryPasswordToString(salt, saltHex, sizeof(salt));
    binaryPasswordToString(hash, hashHex, sizeof(hash));
    return string_format(PASSWORD_HASH_PREFIX "%u$%s$%s", (unsigned)PASSWORD_PBKDF2_ITERATIONS, saltHex, hashHex);
}

bool esp32_authentication::checkStoredPassword(const char *plainPassword, const char *stored, bool &needsUpgrade){
    needsUpgrade = false;
    if(plainPassword == nullptr || stored == nullptr) return false;

    byte expected[SHA256_SIZE];
    byte actual[SHA256_SIZE];

    if(strncmp(stored, PASSWORD_HASH_PREFIX, strlen(PASSWORD_HASH_PREFIX)) == 0){
        // $p2$<iterations>$<salt hex>$<hash hex>
        const char* p = stored + strlen(PASSWORD_HASH_PREFIX);
        char* end = nullptr;
        unsigned long iterations = strtoul(p, &end, 10);
        if(end == p || *end != '$' || iterations == 0 || iterations > 1000000) return false;
        const char* saltHex = end + 1;
        const char* sep = strchr(saltHex, '$');
        if(sep == nullptr) return false;
        size_t saltHexLength = sep - saltHex;
        if(saltHexLength == 0 || saltHexLength % 2 != 0 || saltHexLength > 128) return false;
        const char* hashHex = sep + 1;
        if(strlen(hashHex) != SHA256_SIZE * 2) return false;

        byte salt[64];
        if(!stringPasswordToBinary(saltHex, salt, saltHexLength / 2)) return false;
        if(!stringPasswordToBinary(hashHex, expected, SHA256_SIZE)) return false;
        if(!pbkdf2Sha256(plainPassword, salt, saltHexLength / 2, iterations, actual, SHA256_SIZE)) return false;

        bool valid = constantTimeEquals(expected, actual, SHA256_SIZE);
        needsUpgrade = valid && iterations < PASSWORD_PBKDF2_ITERATIONS;
        return valid;
    }

    // Legacy format: unsalted SHA-256 as 64 hex characters
    if(strlen(stored) != SHA256_SIZE * 2) return false;
    if(!stringPasswordToBinary(stored, expected, SHA256_SIZE)) return false;
    legacySha256(plainPassword, actual);
    bool valid = constantTimeEquals(expected, actual, SHA256_SIZE);
    needsUpgrade = valid;
    return valid;
}

string esp32_authentication::newSessionId(){
    byte sid[SESSION_ID_SIZE];
    char sidHex[SESSION_ID_SIZE * 2 + 1];
    esp_fill_random(sid, sizeof(sid));
    binaryPasswordToString(sid, sidHex, sizeof(sid));
    return string(sidHex);
}

/// @brief Unsalted SHA-256, only used to verify (and then migrate) legacy stored passwords.
void esp32_authentication::legacySha256(const char *plainPassword,byte * output)
{
    esp32_sha256 hasher;
    hasher.init();
    hasher.write(plainPassword);
    memcpy(output,hasher.result(),SHA256_SIZE);
}

#pragma endregion

#pragma region Helper Functions

size_t esp32_authentication::authDocCapacity(){
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    size_t size = 0;
    if(drive->exists(PATH_AUTH_FILE)){
        File f = drive->open(PATH_AUTH_FILE, "r");
        size = f.size();
        f.close();
    }
    return size * 2 + 2048; // headroom for adding a user / upgrading hashes
}

bool esp32_authentication::loadUsers(DynamicJsonDocument &doc){
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    if(!drive->exists(PATH_AUTH_FILE)) return false;
    File authFile = drive->open(PATH_AUTH_FILE,"r");
    DeserializationError error = deserializeJson(doc, authFile);
    authFile.close();
    if(error){
        #if defined(DEBUG_SECURITY) && DEBUG_SECURITY > 0
        Serial.printf("Error occured deserializing authorization file: [%i]%s\n", error.code(), error.c_str());
        #endif
        return false;
    }
    return doc.is<JsonArray>();
}

bool esp32_authentication::saveUsers(DynamicJsonDocument &doc){
    if(doc.overflowed()) return false; // never write a truncated user database
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    File authFile = drive->open(PATH_AUTH_FILE, "w");
    if(!authFile) return false;
    size_t written = serializeJson(doc, authFile);
    authFile.flush();
    authFile.close();
    return written > 0;
}

bool esp32_authentication::constantTimeEquals(const byte* a, const byte* b, size_t length){
    byte diff = 0;
    for(size_t i = 0; i < length; i++)
        diff |= a[i] ^ b[i];
    return diff == 0;
}

void esp32_authentication::binaryPasswordToString(byte *encryptedPass, char *storedPass, size_t length)
{
    for (size_t i = 0; i < length; i++)
        sprintf(storedPass + i*2,"%02X",encryptedPass[i]);
    storedPass[length*2] = '\0';
}

static int hexNibble(char c){
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

bool esp32_authentication::stringPasswordToBinary(const char *storedPass, byte *encryptedPass, size_t length)
{
    for(size_t idx = 0; idx < length; idx++){
        int hi = hexNibble(storedPass[idx*2]);
        if(hi < 0) return false;
        int lo = hexNibble(storedPass[idx*2 + 1]);
        if(lo < 0) return false;
        encryptedPass[idx] = (byte)((hi << 4) | lo);
    }
    return true;
}
#pragma endregion
