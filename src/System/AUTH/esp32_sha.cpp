#include "esp32_sha.h"
 
void esp32_sha256::ProcessInputMessage (string InputString) {     
    // Function ProcessInputMessage
    unsigned long S0; // Sigma0(HashA)  
    unsigned long S1; // Sigma1(HashE)  
    unsigned long maj; // Majority (A,B,C)
    unsigned long ch; // Choose (E,F,G)
    unsigned long temp1, temp2;
    unsigned long w[64]; //Holds input message
    unsigned long s0[64]; //Numbers to generate w[i]
    unsigned long s1[64]; //Number to generate w[i]
    

    int n, LengthOfInputString, ByteIndex, WordIndex = 0;
    int WordPointer, NoOfWordsUsed, BytePosToWriteBit1, ByteInWord2WriteBit1;
    int InputStringBitLength, i; // i use for S0 and S1
    memset(w,0,64);
    memset(s0,0,64);
    memset(s1,0,64);
    unsigned long a, h0=0x6A09E667;
    unsigned long b, h1=0xBB67AE85;
    unsigned long c, h2=0x3C6EF372;
    unsigned long d, h3=0xA54FF53A;
    unsigned long e, h4=0x510E527F;
    unsigned long f, h5=0x9B05688C;
    unsigned long g, h6=0x1F83D9AB;
    unsigned long h, h7=0x5BE0CD19;
    // Initial array of round constants:
    unsigned long k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };
    LengthOfInputString=InputString.length(); //Length in bytes, max 55 bytes

    if (LengthOfInputString > 55) {
        //Serial.println();
        Serial.print("Warning!!! Input strings exceed 55 bytes...");
        //Serial.println();
        return;
    };
    
    WordPointer=LengthOfInputString / 4;//Point to the last word which was written 
    NoOfWordsUsed=WordPointer + 1;

    //Move input strings (ascii) into w[] word by word
    for (WordIndex=0; WordIndex<=NoOfWordsUsed; WordIndex++) {
        for (ByteIndex=0; ByteIndex<=3; ByteIndex++) {
        w[WordIndex]=w[WordIndex]<<8; //Shift left one byte
        ByteInputString=InputString[n];
        w[WordIndex]=w[WordIndex]+ByteInputString;            
        n++; 
        }
    }
    
    //Append '1' bit at (WordPointer and Byte in word to write)
    BytePosToWriteBit1=LengthOfInputString+1;
    ByteInWord2WriteBit1=BytePosToWriteBit1-(WordPointer*4);
    switch (ByteInWord2WriteBit1) {
        case 1:
        w[WordPointer]=w[WordPointer] | 0x80000000;
        break;
        case 2:
        w[WordPointer]=w[WordPointer] | 0x00800000;
        break;
        case 3:
        w[WordPointer]=w[WordPointer] | 0x00008000;
        break;
        case 4:
        w[WordPointer]=w[WordPointer] | 0x00000080;
        break;    
    }
        
    //Write input string bit length to last two words, w[14] and w[15]
    InputStringBitLength=LengthOfInputString*8;
    w[14]=0x0; //This version supports only 55 character of input message, so w[14] is always 0 
    w[15]=w[15]+InputStringBitLength;

    //Serial.print(InputStringBitLength,HEX);
    //Serial.println();

    //Calculate s0[i], s1[i] and w[i]
    for (i=16; i<=63; i++) {
        s0[i]=Gets0(w[i-15]);
        s1[i]=Gets1(w[i-2]);
        w[i]=w[i-16] + s0[i] + w[i-7] + s1[i] ; }  

    //Initialize working variables to current hash value:
    a=h0;
    b=h1;
    c=h2;
    d=h3;
    e=h4;
    f=h5;
    g=h6;
    h=h7;
    
    //Compression function main loop
    for (i=0; i<=63; i++) {
        S1=GetSigma1(e);
        ch=GetChoose(e,f,g);
        temp1=h+S1+ch+k[i]+w[i];
        S0=GetSigma0 (a);
        maj=GetMajority(a,b,c);
        temp2=S0+maj;

        //Assign new hash then loop again
        h=g;
        g=f;
        f=e;
        e=d+temp1;
        d=c;
        c=b;
        b=a;
        a=temp1+temp2;
    }

    //Final hash result, add original hash with the last caculated hash 
    h0=h0+a;
    h1=h1+b;
    h2=h2+c;
    h3=h3+d;
    h4=h4+e;
    h5=h5+f;
    h6=h6+g;
    h7=h7+h;

    Serial.println() ;
    Serial.print("SHA256 hashed output message...") ;
    Serial.println() ;

    //Print the result
    PrintWithLeadingZero(h0);
    PrintWithLeadingZero(h1);
    PrintWithLeadingZero(h2);
    PrintWithLeadingZero(h3);
    PrintWithLeadingZero(h4);
    PrintWithLeadingZero(h5);
    PrintWithLeadingZero(h6);
    PrintWithLeadingZero(h7);
    Serial.println();
    Serial.print("----------------------------------------------------------------") ;
    Serial.println();
  
}//End
