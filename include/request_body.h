#ifndef _REQUEST_BODY_H
#define _REQUEST_BODY_H
// Reads a request body completely.
//
// Handlers used to read until a read returned nothing. Over HTTPS the body often arrives in a
// later TLS record than the headers, so that loop could stop before any of it had arrived and
// the handler parsed an empty or partial body (a settings save that never completed, a login
// refused as "no credentials"). This reads exactly Content-Length bytes, waiting briefly for
// the rest to arrive.
#include <Arduino.h>
#include <string>
#include <HTTPRequest.hpp>

static inline bool readRequestBody(httpsserver::HTTPRequest* req, std::string& out,
                                   size_t maxBytes = 32768, unsigned long timeoutMs = 5000) {
    out.clear();
    size_t expected = req->getContentLength();
    if (expected == 0) return true;
    if (expected > maxBytes) return false;
    out.reserve(expected);
    char buf[256];
    unsigned long start = millis();
    while (out.size() < expected && millis() - start < timeoutMs) {
        size_t want = expected - out.size();
        if (want > sizeof(buf)) want = sizeof(buf);
        size_t got = req->readBytes((byte*)buf, want);
        if (got == 0) {
            if (req->requestComplete()) break;
            delay(2);
            continue;
        }
        out.append(buf, got);
    }
    return out.size() == expected;
}
#endif
