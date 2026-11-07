#ifndef _ESP32_DIAG_H
#define _ESP32_DIAG_H
// Diagnostics (build with -DSERVER_DIAGNOSTICS): records where the device task is,
// so the server heartbeat can report a task stuck while holding the shared lock.
#ifdef SERVER_DIAGNOSTICS
#include <Arduino.h>
extern volatile const char* g_diagWhere;
extern volatile unsigned long g_diagSince;
#define DIAG_AT(label) do { g_diagWhere = (label); g_diagSince = millis(); } while(0)
extern volatile const char* g_diagLcdWhere;
extern volatile unsigned long g_diagLcdSince;
#define DIAG_LCD(label) do { g_diagLcdWhere = (label); g_diagLcdSince = millis(); } while(0)
#else
#define DIAG_AT(label) do {} while(0)
#define DIAG_LCD(label) do {} while(0)
#endif
#endif
