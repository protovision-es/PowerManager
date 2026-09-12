#ifndef LOG_H
#define LOG_H

#include <Arduino.h>
#include <time.h>

#define LOG_MAX 50

struct LogEntry {
    char time[20];
    char msg[96];
};

static LogEntry logBuffer[LOG_MAX];
static int logHead = 0;
static int logCount = 0;

static void logGetTime(char *buf, int len) {
    struct tm t;
    if (getLocalTime(&t)) {
        snprintf(buf, len, "%02d/%02d %02d:%02d:%02d",
            t.tm_mday, t.tm_mon + 1, t.tm_hour, t.tm_min, t.tm_sec);
    } else {
        snprintf(buf, len, "??:??");
    }
}

inline void logAdd(const char *msg) {
    LogEntry &e = logBuffer[logHead];
    logGetTime(e.time, sizeof(e.time));
    strlcpy(e.msg, msg, sizeof(e.msg));
    logHead = (logHead + 1) % LOG_MAX;
    if (logCount < LOG_MAX) logCount++;
}

inline String logJson() {
    String json = "[";
    int start = (logCount < LOG_MAX) ? 0 : logHead;
    for (int i = 0; i < logCount; i++) {
        int idx = (start + i) % LOG_MAX;
        if (i > 0) json += ",";
        json += "{\"t\":\"" + String(logBuffer[idx].time) + "\"";
        json += ",\"m\":\"" + String(logBuffer[idx].msg) + "\"}";
    }
    json += "]";
    return json;
}

#endif
