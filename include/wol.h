#ifndef WOL_H
#define WOL_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include "config.h"
#include "storage.h"
#include "log.h"
#include "util.h"

static WiFiUDP wolUDP;
static WakeOnLan WOL(wolUDP);
static unsigned long wolLastSent[WOL_MAX_DEVICES] = {0};

inline void wolInit() {
    WOL.setRepeat(WOL_SEND_REPEAT, WOL_SEND_DELAY);
    Serial.println("[WoL] Initialized");
}

// Sends the magic packet. Must be called from loop() context (see actions.h):
// it blocks for WOL_SEND_REPEAT * WOL_SEND_DELAY ms.
inline bool wolSend(int id) {
    if (id < 0 || id >= appConfig.wolCount) return false;
    const WoLDevice &d = appConfig.wolDevices[id];
    if (WiFi.status() != WL_CONNECTED) {
        logAddf("WoL to %s failed: WiFi not connected", d.name);
        return false;
    }
    // Recomputed on every send: the address is wrong if WiFi was not
    // connected yet at boot, or if the network changed since.
    WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());
    bool ok = WOL.sendMagicPacket(d.mac);
    if (ok) {
        unsigned long now = millis();
        wolLastSent[id] = now ? now : 1;
        logAddf("WoL magic packet sent to %s", d.name);
    } else {
        logAddf("WoL to %s failed (bad MAC %s?)", d.name, d.mac);
    }
    return ok;
}

inline bool wolValidId(int id) {
    return id >= 0 && id < appConfig.wolCount;
}

inline String wolStatusJson() {
    String json = "[";
    for (int i = 0; i < appConfig.wolCount; i++) {
        if (i > 0) json += ",";
        json += "{\"id\":" + String(i);
        json += ",\"name\":\"" + jsonEscape(appConfig.wolDevices[i].name) + "\"";
        json += ",\"mac\":\"" + jsonEscape(appConfig.wolDevices[i].mac) + "\"";
        json += ",\"lastAgo\":" + String(secondsSince(wolLastSent[i]));
        json += "}";
    }
    json += "]";
    return json;
}

#endif
