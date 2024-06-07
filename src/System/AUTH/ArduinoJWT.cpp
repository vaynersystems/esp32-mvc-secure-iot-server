/**

 Copyright (c) 2016, Interior Automation Ltd.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation and/or
    other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be
    used to endorse or promote products derived from this software without specific prior
    written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 **/
#define DEBUG
#include <Arduino.h>
#include "ArduinoJWT.h"
#include "base64.hpp"
#include "esp32_sha256.h"
#include "string_helper.h"

// The standard JWT header already base64 encoded. Equates to {"alg": "HS256", "typ": "JWT"}
const PROGMEM char* jwtHeader = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";

ArduinoJWT::ArduinoJWT(string psk) {
  _psk = psk;
}

ArduinoJWT::ArduinoJWT(char* psk) {
  _psk = string(psk);
}

void ArduinoJWT::setPSK(string psk) {
  _psk = psk;
}
void ArduinoJWT::setPSK(char* psk) {
  _psk = string(psk);
}

int ArduinoJWT::getJWTLength(string& payload) {
  return getJWTLength((char*)payload.c_str());
}

int ArduinoJWT::getJWTLength(char* payload) {
  return strlen(jwtHeader) + encode_base64_length(strlen(payload)) + encode_base64_length(32) + 2;
}

int ArduinoJWT::getJWTPayloadLength(string& jwt) {
  return getJWTPayloadLength((char*)jwt.c_str());
}

int ArduinoJWT::getJWTPayloadLength(const char* jwt) {
    auto tokens = explode(jwt,".");
    //Serial.printf("Found %d tokens in JWT payload\n", tokens.size());
    if(tokens.size() < 3) return -1;
    return decode_base64_length((unsigned char*)tokens[1].c_str()) + 1;
//   char jwtCopy[strlen(jwt)];
//   memcpy((char*)jwtCopy, jwt, strlen(jwt));
    // Get all three jwt parts
//   const char* sep = ".";
//   char* token;
//   token = strtok((char*)jwt, sep);
//   token = strtok(NULL, sep);
//   if(token == NULL) {
//     return -1;
//   } else {
//     return decode_base64_length((unsigned char*)token) + 1;
//   }
}

string ArduinoJWT::encodeJWT(string& payload) {
  char jwt[getJWTLength(payload)];
  encodeJWT((char*)payload.c_str(), (char*)jwt);
  return string(jwt);
}

void ArduinoJWT::encodeJWT(char* payload, char* jwt) {
  unsigned char* ptr = (unsigned char*)jwt;
  // Build the initial part of the jwt (header.payload)
  memcpy(ptr, jwtHeader, strlen(jwtHeader));
  ptr += strlen(jwtHeader);
  *ptr++ = '.';
  encode_base64((unsigned char*)payload, strlen(payload), ptr);
  ptr += encode_base64_length(strlen(payload));
  // Get rid of any padding (trailing '=' added when base64 encoding)
  while(*(ptr - 1) == '=') {
    ptr--;
  }
  *(ptr) = 0;
  // Build the signature
  Sha256.initHmac((const unsigned char*)_psk.c_str(), _psk.length());
  Sha256.print(jwt);
  // Add the signature to the jwt
  *ptr++ = '.';
  encode_base64(Sha256.resultHmac(), 32, ptr);
  ptr += encode_base64_length(32);
  // Get rid of any padding and replace / and +
  while(*(ptr - 1) == '=') {
    ptr--;
  }
  *(ptr) = 0;
}

bool ArduinoJWT::decodeJWT(string& jwt,  string& payload) {
    try{
        //int payloadLength = getJWTPayloadLength(jwt.c_str());
        //if(payloadLength > 0) {
            //Serial.printf("\t\t PAYLOAD LENGTH: %d\n", payloadLength);
            //char* jsonPayload = (char*)malloc(payloadLength);
            // if(jsonPayload == NULL) {
            //     Serial.println("Failed to allocate memory to decode JWT Token");
            //     return false;
            // }
            //char jsonPayload[payloadLength];
            Serial.printf("Received token string \n%s\n for processing\n", jwt.c_str());
            if(decodeJWT(jwt.c_str(), _payload, getJWTPayloadLength(jwt.c_str()))) {
                payload = _payload;
#ifdef DEBUG
                Serial.printf("Sucessfully decoded JWT token payload: [%s]\n",payload.c_str());
#endif      //free(jsonPayload);
            return true;
            }
#ifdef DEBUG
            Serial.printf("Failed to decoded JWT token: [%s]\n", jwt.c_str());
#endif
//         }
//         else {
// #ifdef DEBUG
//             Serial.printf("Failed to decoded JWT token with 0 length!\n");
// #endif   
//         }
    }
    catch(...){

    }
    return false;
}

bool ArduinoJWT::decodeJWT(const char* jwt, char* payload, int payloadLength) {
    // Get all three jwt parts
    Serial.printf("Received token \n%s\n for processing\n", jwt);
    auto tokens = explode(jwt,".");    
    if(tokens.size() < 3) {
        #ifdef DEBUG
        Serial.printf("Missing critical section of token {Header:Payload:Signature}, {%s,\t%s,\t%s}\n in token: %s\n",
            tokens.size() > 0 ? tokens[0].c_str() : "MISSING",
            tokens.size() > 1 ? tokens[1].c_str() : "MISSING",
            tokens.size() > 2 ? tokens[2].c_str() : "MISSING",
            jwt
        );
        #endif
        payload = NULL;
        return false;
    }
//   const char* sep = ".";
//   char* encodedHeader = strtok((char*)jwt, sep);
//   char* encodedPayload = strtok(NULL, sep);
//   char* encodedSignature = strtok(NULL, sep);

  // Check all three jwt parts exist
//   if(encodedHeader == NULL || encodedPayload == NULL || encodedSignature == NULL)
//   {

//     payload = NULL;
//     return false;
//   }

  // Build the signature
  Sha256.initHmac((const unsigned char*)_psk.c_str(), _psk.length());
  Sha256.print(tokens[0].c_str());
  Sha256.print(".");
  Sha256.print(tokens[1].c_str());

  // Encode the signature as base64
  unsigned char base64Signature[encode_base64_length(32)];
  encode_base64(Sha256.resultHmac(), 32, base64Signature);
  unsigned char* ptr = &base64Signature[0] + encode_base64_length(32);
  // Get rid of any padding and replace / and +
  while(*(ptr - 1) == '=') {
    ptr--;
  }
  *(ptr) = 0;

  // Do the signatures match?
  if(strcmp((char*)tokens[2].c_str(), (char*)base64Signature) == 0) {
    // Decode the payload
    decode_base64((unsigned char*)tokens[1].c_str(), (unsigned char*)payload);
    payload[payloadLength - 1] = 0;
    return true;
  } else {
#ifdef DEBUG
      Serial.printf("Failed to validate JWT Signature \nActual\t[%s] \nExpected\t[%s]\n", tokens[2].c_str(), (unsigned char*)base64Signature);
#endif
    payload = NULL;
    return false;
  }
}
