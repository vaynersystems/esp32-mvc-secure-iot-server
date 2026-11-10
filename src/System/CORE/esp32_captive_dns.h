#ifndef _ESP32_CAPTIVE_DNS_H
#define _ESP32_CAPTIVE_DNS_H
// Captive-portal DNS answer builder: every name resolves to the device.
// Pure function (no sockets) so it can be tested off-device.
//
// Why not Arduino's DNSServer: in core 2.0.x it only answers queries with no additional
// records. Android and iOS add an EDNS0 OPT record to their queries, so every lookup
// got "no error, no address", the OS connectivity probe never reached the device, and
// no "sign in to network" prompt appeared.
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define CAPTIVE_DNS_TTL 10          // seconds; short so names re-resolve after setup
#define CAPTIVE_DNS_TYPE_A 1
#define CAPTIVE_DNS_TYPE_ANY 255
#define CAPTIVE_DNS_CLASS_IN 1

/// @brief Build the reply to one DNS query.
/// @param query   received packet
/// @param ipv4    address to answer with, in network byte order as 4 bytes (a.b.c.d)
/// @param name    optional: receives the queried name as dotted text (for logging)
/// @param qtype   optional: receives the query type
/// @return reply length, or 0 if the packet should be ignored (not a standard query / malformed)
static size_t captive_dns_build_reply(const uint8_t* query, size_t queryLength, const uint8_t ipv4[4],
                                      uint8_t* reply, size_t replyCapacity,
                                      char* name = nullptr, size_t nameCapacity = 0, uint16_t* qtype = nullptr){
    if(name && nameCapacity) name[0] = 0;
    if(queryLength < 12 || replyCapacity < 12) return 0;
    uint8_t flags1 = query[2];
    bool isQuery = (flags1 & 0x80) == 0;
    uint8_t opcode = (flags1 >> 3) & 0x0F;
    uint16_t qdcount = (uint16_t)((query[4] << 8) | query[5]);
    if(!isQuery || opcode != 0 || qdcount < 1) return 0;

    // First question: QNAME labels (queries never use compression), then QTYPE, QCLASS.
    size_t pos = 12, namePos = 0;
    while(true){
        if(pos >= queryLength) return 0;
        uint8_t labelLength = query[pos];
        if(labelLength == 0){ pos++; break; }
        if(labelLength & 0xC0) return 0; // compression pointer: not valid in a question
        if(pos + 1 + labelLength > queryLength) return 0;
        if(name && nameCapacity){
            if(namePos && namePos + 1 < nameCapacity) name[namePos++] = '.';
            for(uint8_t i = 0; i < labelLength && namePos + 1 < nameCapacity; i++)
                name[namePos++] = (char)query[pos + 1 + i];
            name[namePos] = 0;
        }
        pos += 1 + labelLength;
        if(pos - 12 > 255) return 0;
    }
    if(pos + 4 > queryLength) return 0;
    uint16_t type = (uint16_t)((query[pos] << 8) | query[pos + 1]);
    uint16_t qclass = (uint16_t)((query[pos + 2] << 8) | query[pos + 3]);
    size_t questionEnd = pos + 4;
    if(qtype) *qtype = type;

    bool answer = (type == CAPTIVE_DNS_TYPE_A || type == CAPTIVE_DNS_TYPE_ANY) &&
                  (qclass == CAPTIVE_DNS_CLASS_IN || qclass == 255);
    size_t replyLength = questionEnd + (answer ? 16 : 0);
    if(replyLength > replyCapacity) return 0;

    // Header: same ID; QR=1, same opcode, AA=1, same RD; RA=0, RCODE=0 (NOERROR).
    // One question, one answer (or none: NODATA for AAAA/HTTPS/... so the client uses A).
    // EDNS is not echoed, which simply tells the client this server does not use it.
    memcpy(reply, query, questionEnd);
    reply[2] = (uint8_t)(0x80 | (opcode << 3) | 0x04 | (flags1 & 0x01));
    reply[3] = 0x00;
    reply[4] = 0; reply[5] = 1;
    reply[6] = 0; reply[7] = answer ? 1 : 0;
    reply[8] = 0; reply[9] = 0;
    reply[10] = 0; reply[11] = 0;

    if(answer){
        uint8_t* a = reply + questionEnd;
        a[0] = 0xC0; a[1] = 0x0C;                 // name: pointer to the question
        a[2] = 0; a[3] = CAPTIVE_DNS_TYPE_A;
        a[4] = 0; a[5] = CAPTIVE_DNS_CLASS_IN;
        a[6] = 0; a[7] = 0; a[8] = 0; a[9] = CAPTIVE_DNS_TTL;
        a[10] = 0; a[11] = 4;
        memcpy(a + 12, ipv4, 4);
    }
    return replyLength;
}

#endif
