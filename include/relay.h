#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>
#include "config.h"
#include "storage.h"
#include "log.h"

inline void relayInit() {
    for (int i = 0; i < RELAY_COUNT; i++) {
        pinMode(appConfig.relays[i].pin, OUTPUT);
        digitalWrite(appConfig.relays[i].pin, RELAY_ACTIVE);
        appConfig.relays[i].restarting = false;
        appConfig.relays[i].restartStart = 0;
        Serial.printf("[Relay] %s (GPIO %d) -> ON (%ds restart)\n",
            appConfig.relays[i].name,
            appConfig.relays[i].pin,
            appConfig.relays[i].restartSec);
    }
}

inline void relayRestart(int id) {
    if (id < 0 || id >= RELAY_COUNT) return;
    if (appConfig.relays[id].restarting) return;

    appConfig.relays[id].restarting = true;
    appConfig.relays[id].restartStart = millis();
    digitalWrite(appConfig.relays[id].pin, RELAY_INACTIVE);
    char buf[64];
    snprintf(buf, sizeof(buf), "Relay %s restarting (OFF %ds)", appConfig.relays[id].name, appConfig.relays[id].restartSec);
    logAdd(buf);
    Serial.printf("[Relay] %s -> RESTARTING (OFF %ds)\n",
        appConfig.relays[id].name,
        appConfig.relays[id].restartSec);
}

inline void relayUpdate() {
    for (int i = 0; i < RELAY_COUNT; i++) {
        if (!appConfig.relays[i].restarting) continue;

        unsigned long elapsed = (millis() - appConfig.relays[i].restartStart) / 1000;
        if (elapsed >= (unsigned long)appConfig.relays[i].restartSec) {
            digitalWrite(appConfig.relays[i].pin, RELAY_ACTIVE);
            appConfig.relays[i].restarting = false;
            char buf[64];
            snprintf(buf, sizeof(buf), "Relay %s ON (restart complete)", appConfig.relays[i].name);
            logAdd(buf);
            Serial.printf("[Relay] %s -> ON (restart complete)\n",
                appConfig.relays[i].name);
        }
    }
}

inline bool relayAnyRestarting() {
    for (int i = 0; i < RELAY_COUNT; i++) {
        if (appConfig.relays[i].restarting) return true;
    }
    return false;
}

inline int relayGetRemaining(int id) {
    if (id < 0 || id >= RELAY_COUNT) return 0;
    if (!appConfig.relays[id].restarting) return 0;
    unsigned long elapsed = (millis() - appConfig.relays[id].restartStart) / 1000;
    int remain = appConfig.relays[id].restartSec - (int)elapsed;
    return remain > 0 ? remain : 0;
}

inline String relayStatusJson() {
    String json = "[";
    for (int i = 0; i < RELAY_COUNT; i++) {
        if (i > 0) json += ",";
        bool on = !appConfig.relays[i].restarting;
        int remaining = relayGetRemaining(i);
        json += "{\"id\":" + String(i);
        json += ",\"name\":\"" + String(appConfig.relays[i].name) + "\"";
        json += ",\"pin\":" + String(appConfig.relays[i].pin);
        json += ",\"restartSec\":" + String(appConfig.relays[i].restartSec);
        json += ",\"on\":" + String(on ? "true" : "false");
        json += ",\"restarting\":" + String(appConfig.relays[i].restarting ? "true" : "false");
        json += ",\"remaining\":" + String(remaining);
        json += "}";
    }
    json += "]";
    return json;
}

#endif
