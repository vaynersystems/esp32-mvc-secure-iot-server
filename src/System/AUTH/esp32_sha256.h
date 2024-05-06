/**
 * Based on original work by Cathedrow
 * Original library located at https://github.com/Cathedrow/Cryptosuite
 */

#ifndef _ESP32_SHA_H_
#define _ESP32_SHA_H_

#include <inttypes.h>
#include "Print.h"

#define SHA256_SIZE 32
#define BLOCK_LENGTH 64

union _buffer {
  unsigned char b[BLOCK_LENGTH];
  uint32_t w[BLOCK_LENGTH/4];
};
union _state {
  unsigned char b[SHA256_SIZE];
  uint32_t w[SHA256_SIZE/4];
};

class esp32_sha256 : public Print
{
  public:
    void init(void);
    void initHmac(const unsigned char* secret, int secretLength);
    unsigned char* result(void);
    unsigned char* resultHmac(void);
    virtual size_t write(unsigned char);
    using Print::write;
  private:
    void pad();
    void addUncounted(unsigned char data);
    void hashBlock();
    uint32_t ror32(uint32_t number, unsigned char bits);
    _buffer buffer;
    unsigned char bufferOffset;
    _state state;
    uint32_t byteCount;
    unsigned char keyBuffer[BLOCK_LENGTH];
    unsigned char innerHash[SHA256_SIZE];
};
extern esp32_sha256 Sha256;

#endif
