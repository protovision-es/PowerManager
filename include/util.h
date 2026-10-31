#ifndef UTIL_H
#define UTIL_H

#include <Arduino.h>
#include <time.h>
#include <esp_timer.h>

// Uptime in seconds from a 64-bit timer (millis() wraps after ~49.7 days).
inline uint32_t uptimeSec() {
    return (uint32_t)(esp_timer_get_time() / 1000000ULL);
}

// Non-blocking local time. getLocalTime() waits up to 5 s by default when
// the clock has not been set yet (no NTP), which would stall the loop and the
// web server; with a 0 ms timeout it returns false immediately instead.
inline bool timeNow(struct tm &t) {
    return getLocalTime(&t, 0);
}

inline bool timeSynced() {
    struct tm t;
    return timeNow(t);
}

// Escapes a string so it can be embedded inside a JSON string literal.
inline String jsonEscape(const char *s) {
    String out;
    if (!s) return out;
    out.reserve(strlen(s) + 4);
    for (const char *p = s; *p; p++) {
        char c = *p;
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if ((uint8_t)c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

// Escapes a string for safe insertion into HTML text or attributes.
inline String htmlEscape(const char *s) {
    String out;
    if (!s) return out;
    out.reserve(strlen(s) + 8);
    for (const char *p = s; *p; p++) {
        switch (*p) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += *p;
        }
    }
    return out;
}

// Seconds elapsed since a millis() timestamp (wrap-safe), or -1 if ts == 0.
inline long secondsSince(unsigned long ts) {
    if (ts == 0) return -1;
    return (long)((millis() - ts) / 1000UL);
}

#endif
