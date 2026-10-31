#ifndef LOG_H
#define LOG_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "util.h"

#define LOG_MAX 50

struct LogEntry {
    char time[20];
    char msg[96];
};

static LogEntry logBuffer[LOG_MAX];
static int logHead = 0;
static int logCount = 0;
static uint32_t logSeq = 0;              // increments on every new entry (32-bit reads are atomic)
static SemaphoreHandle_t logMutex = nullptr;

// The log is written from loop() and read from the web server task,
// so access is serialized with a mutex. Call once from setup().
inline void logInit() {
    if (!logMutex) logMutex = xSemaphoreCreateMutex();
}

static void logGetTime(char *buf, int len) {
    struct tm t;
    if (timeNow(t)) {
        snprintf(buf, len, "%02d/%02d %02d:%02d:%02d",
            t.tm_mday, t.tm_mon + 1, t.tm_hour, t.tm_min, t.tm_sec);
    } else {
        // Clock not synced yet: show uptime instead of "??:??"
        snprintf(buf, len, "+%lus", (unsigned long)uptimeSec());
    }
}

inline void logAdd(const char *msg) {
    if (!logMutex) logInit();
    xSemaphoreTake(logMutex, portMAX_DELAY);
    LogEntry &e = logBuffer[logHead];
    logGetTime(e.time, sizeof(e.time));
    strlcpy(e.msg, msg, sizeof(e.msg));
    logHead = (logHead + 1) % LOG_MAX;
    if (logCount < LOG_MAX) logCount++;
    logSeq++;
    xSemaphoreGive(logMutex);
    Serial.printf("[Log] %s\n", msg);
}

inline void logAddf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
inline void logAddf(const char *fmt, ...) {
    char buf[96];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    logAdd(buf);
}

inline uint32_t logGetSeq() {
    return logSeq;
}

inline String logJson() {
    if (!logMutex) logInit();
    String json;
    json.reserve(logCount * 72 + 2);
    json = "[";
    xSemaphoreTake(logMutex, portMAX_DELAY);
    int start = (logCount < LOG_MAX) ? 0 : logHead;
    for (int i = 0; i < logCount; i++) {
        int idx = (start + i) % LOG_MAX;
        if (i > 0) json += ",";
        json += "{\"t\":\"";
        json += jsonEscape(logBuffer[idx].time);
        json += "\",\"m\":\"";
        json += jsonEscape(logBuffer[idx].msg);
        json += "\"}";
    }
    xSemaphoreGive(logMutex);
    json += "]";
    return json;
}

#endif
