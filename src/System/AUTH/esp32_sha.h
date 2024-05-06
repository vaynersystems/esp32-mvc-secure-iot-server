#ifndef _ESP32_SHA_H
#define _ESP32_SHA_H
#include "Arduino.h"
#include <string>
#include <cstddef>
using namespace std;

class esp32_sha256{
    public:
     void ProcessInputMessage (string InputString);  

    private:
    
    uint8_t ByteInputString;
    // Function GetS1 (s1[i])
     unsigned long s1Return;
     unsigned long Gets1 (unsigned long wi2Receive) {
    s1Return=((wi2Receive & 0x0001ffff)<<32-17)+(wi2Receive>>17)^((wi2Receive & 0x0007ffff)<<32-19)+(wi2Receive>>19)^(wi2Receive>>10);
    return s1Return;}

    // Function Gets0[i]
    unsigned long s0Return;
    unsigned long Gets0 (unsigned long wi15Receive) {
        s0Return=((wi15Receive&0x0000007f)<<32-7)+(wi15Receive>>7)^((wi15Receive&0x0003ffff)<<32-18)+(wi15Receive>>18)^(wi15Receive>>3);
        return s0Return;
    }

    // Function GetChoose(E,F,G)
    unsigned long Choose ; 
    unsigned long GetChoose (unsigned long HashE,unsigned long HashF,unsigned long HashG) {
        Choose=(HashE & HashF)^((~HashE)&HashG);  
        return Choose;
    }
        
    // Function GetMajority
    unsigned long Majority; //Majority(A,B,C)
    unsigned long GetMajority (unsigned long HashA,unsigned long HashB,unsigned long HashC) {
        Majority=(HashA&HashB)^(HashA&HashC)^(HashB&HashC);
        return Majority;
    }

    // Function GetSigma0
    unsigned long TempSigma0 ;
    unsigned long GetSigma0 (unsigned long InputHash) {
        TempSigma0=((InputHash&0x00000003)<<32-2)+(InputHash>>2)^((InputHash&0x00001fff)<<32-13)+(InputHash>>13)^((InputHash&0x003fffff)<<32-22)+(InputHash>>22);
        return TempSigma0;
    }

    // Function GetSigma1
    unsigned long TempSigma1 ;
    unsigned long GetSigma1 (unsigned long InputHash) {
        TempSigma1=((InputHash&0x0000003f)<<32-6)+(InputHash>>6)^((InputHash&0x000007ff)<<32-11)+(InputHash>>11)^((InputHash&0x01ffffff)<<32-25)+(InputHash>>25);
        return TempSigma1;
    }

    // Function PrintWithLeadingZero
    unsigned long TempLz;
    void PrintWithLeadingZero (unsigned long ReceiveUnsignedLong) {
        TempLz=ReceiveUnsignedLong&0xf0000000; TempLz=TempLz>>28; Serial.print(TempLz, HEX);
        TempLz=ReceiveUnsignedLong&0x0f000000; TempLz=TempLz>>24; Serial.print(TempLz, HEX);
        TempLz=ReceiveUnsignedLong&0x00f00000; TempLz=TempLz>>20; Serial.print(TempLz, HEX);
        TempLz=ReceiveUnsignedLong&0x000f0000; TempLz=TempLz>>16; Serial.print(TempLz, HEX);
        TempLz=ReceiveUnsignedLong&0x0000f000; TempLz=TempLz>>12; Serial.print(TempLz, HEX);
        TempLz=ReceiveUnsignedLong&0x00000f00; TempLz=TempLz>>8; Serial.print(TempLz, HEX);
        TempLz=ReceiveUnsignedLong&0x000000f0; TempLz=TempLz>>4; Serial.print(TempLz, HEX);
        TempLz=ReceiveUnsignedLong&0x0000000f; TempLz=TempLz>>0; Serial.print(TempLz, HEX);
    }
};

#endif