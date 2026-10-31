#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>
#include "config.h"
#include "storage.h"
#include "log.h"
#include "util.h"

// Call as early as possible in setup(): until the pins are driven, the relays
// sit in their inactive state, which powers the equipment off.
inline void relayInit() {
    for (int i = 0; i < RELAY_COUNT; i++) {
        digitalWrite(appConfig.relays[i].pin, RELAY_ACTIVE);   // set level before enabling output
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

// Starts a power cycle. Must be called from loop() context (see actions.h).
// Returns false if the id is invalid or that relay is already restarting.
inline bool relayRestart(int id, const char *reason) {
    if (id < 0 || id >= RELAY_COUNT) return false;
    RelayConfig &r = appConfig.relays[id];
    if (r.restarting) return false;

    unsigned long now = millis();
    r.restarting = true;
    r.restartStart = now ? now : 1;
    r.lastRestart = r.restartStart;
    r.lastReason = reason ? reason : "";
    if (r.restartCount < 0xFFFF) r.restartCount++;
    digitalWrite(r.pin, RELAY_INACTIVE);
    logAddf("%s: power OFF for %ds (%s)", r.name, r.restartSec, r.lastReason);
    return true;
}

// Called every loop(); turns relays back on when their time is up.
inline void relayUpdate() {
    unsigned long now = millis();
    for (int i = 0; i < RELAY_COUNT; i++) {
        RelayConfig &r = appConfig.relays[i];
        if (!r.restarting) continue;
        if (now - r.restartStart >= (unsigned long)r.restartSec * 1000UL) {
            digitalWrite(r.pin, RELAY_ACTIVE);
            r.restarting = false;
            logAddf("%s: power back ON", r.name);
        }
    }
}

inline bool relayAnyRestarting() {
    for (int i = 0; i < RELAY_COUNT; i++) {
        if (appConfig.relays[i].restarting) return true;
    }
    return false;
}

// Seconds left in the current restart, rounded up (0 if not restarting).
inline int relayGetRemaining(int id) {
    if (id < 0 || id >= RELAY_COUNT) return 0;
    const RelayConfig &r = appConfig.relays[id];
    if (!r.restarting) return 0;
    unsigned long elapsed = millis() - r.restartStart;
    unsigned long total = (unsigned long)r.restartSec * 1000UL;
    if (elapsed >= total) return 0;
    return (int)((total - elapsed + 999UL) / 1000UL);
}

inline String relayStatusJson() {
    String json = "[";
    for (int i = 0; i < RELAY_COUNT; i++) {
        const RelayConfig &r = appConfig.relays[i];
        if (i > 0) json += ",";
        json += "{\"id\":" + String(i);
        json += ",\"name\":\"" + jsonEscape(r.name) + "\"";
        json += ",\"pin\":" + String(r.pin);
        json += ",\"restartSec\":" + String(r.restartSec);
        json += ",\"on\":" + String(r.restarting ? "false" : "true");
        json += ",\"restarting\":" + String(r.restarting ? "true" : "false");
        json += ",\"remaining\":" + String(relayGetRemaining(i));
        json += ",\"restarts\":" + String(r.restartCount);
        json += ",\"lastAgo\":" + String(secondsSince(r.lastRestart));
        json += ",\"lastReason\":\"" + jsonEscape(r.lastReason) + "\"";
        json += "}";
    }
    json += "]";
    return json;
}

#endif
