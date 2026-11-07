#ifndef _ESP32_SYSTEM_HELPER_H
#define _ESP32_SYSTEM_HELPER_H
#include <string>
using namespace std;

static tm getDate(){
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    // Set timezone to Eastern Standard Time
    // setenv("TZ", "EST+5", 1);
    // tzset();
    localtime_r(&now, &timeinfo);
    return timeinfo;
}
static string getCurrentTime(){
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    // Set timezone to Eastern Standard Time
    // setenv("TZ", "EST+5", 1);
    // tzset();

    localtime_r(&now, &timeinfo);

    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    return string(strftime_buf);
};

// Current epoch time, or 0 if the clock has not been set (no NTP yet, e.g. in access-point mode).
// Never waits: getLocalTime() defaults to retrying for 5 s, and callers hold the shared task lock,
// which starved the web server whenever the clock was unset.
static unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 0)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}
#endif