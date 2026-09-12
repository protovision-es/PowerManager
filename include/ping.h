#ifndef PING_H
#define PING_H

#include <WiFi.h>
#include <WiFiClient.h>
#include "config.h"
#include "storage.h"
#include "relay.h"
#include "log.h"

static int pingFailCount = 0;
static unsigned long lastPingTime = 0;
static bool lastPingResult = true;
static int lastPingMs = 0;

inline void pingInit() {
    pingFailCount = 0;
    lastPingTime = 0;
    lastPingResult = true;
    lastPingMs = 0;
    Serial.printf("[Ping] Host: %s, Interval: %ds, Threshold: %d\n",
        appConfig.ping.host,
        appConfig.ping.intervalSec,
        appConfig.ping.failThreshold);
}

static bool pingHost(const char* host, int count, int &outMs) {
    IPAddress ip;
    if (!WiFi.hostByName(host, ip)) { outMs = 0; return false; }

    int success = 0;
    int totalMs = 0;
    for (int i = 0; i < count; i++) {
        unsigned long start = millis();
        WiFiClient client;
        if (client.connect(host, 53, 1000)) {
            client.stop();
            totalMs += millis() - start;
            success++;
        }
        delay(10);
    }
    outMs = success > 0 ? totalMs / success : 0;
    return success > 0;
}

inline void pingUpdate() {
    unsigned long now = millis();
    if (now - lastPingTime < (unsigned long)appConfig.ping.intervalSec * 1000) return;
    lastPingTime = now;

    int ms = 0;
    bool success = pingHost(appConfig.ping.host, 3, ms);
    lastPingResult = success;
    lastPingMs = ms;

    if (success) {
        pingFailCount = 0;
        Serial.printf("[Ping] OK (%dms)\n", ms);
    } else {
        pingFailCount++;
        Serial.printf("[Ping] FAIL (%d/%d)\n", pingFailCount, appConfig.ping.failThreshold);

        if (pingFailCount >= appConfig.ping.failThreshold) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Ping failed %d times, restarting %s", pingFailCount, appConfig.relays[appConfig.ping.relayTrigger].name);
            logAdd(buf);
            Serial.printf("[Ping] Restarting %s due to internet failure\n",
                appConfig.relays[appConfig.ping.relayTrigger].name);
            relayRestart(appConfig.ping.relayTrigger);
            pingFailCount = 0;
        }
    }
}

inline String pingStatusJson() {
    String json = "{";
    json += "\"host\":\"" + String(appConfig.ping.host) + "\"";
    json += ",\"interval\":" + String(appConfig.ping.intervalSec);
    json += ",\"threshold\":" + String(appConfig.ping.failThreshold);
    json += ",\"relayTrigger\":" + String(appConfig.ping.relayTrigger);
    json += ",\"failCount\":" + String(pingFailCount);
    json += ",\"lastResult\":" + String(lastPingResult ? "true" : "false");
    json += ",\"ms\":" + String(lastPingMs);
    json += "}";
    return json;
}

#endif
