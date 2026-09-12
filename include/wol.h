#ifndef WOL_H
#define WOL_H

#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include "config.h"
#include "storage.h"
#include "log.h"

static WiFiUDP wolUDP;
static WakeOnLan WOL(wolUDP);

inline void wolInit() {
    WOL.setRepeat(WOL_SEND_REPEAT, WOL_SEND_DELAY);
    WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());
    Serial.println("[WoL] Initialized");
}

inline bool wolSend(int id) {
    if (id < 0 || id >= appConfig.wolCount) return false;
    char buf[64];
    snprintf(buf, sizeof(buf), "WoL sent to %s", appConfig.wolDevices[id].name);
    logAdd(buf);
    Serial.printf("[WoL] Sending magic packet to %s (%s)\n",
        appConfig.wolDevices[id].name,
        appConfig.wolDevices[id].mac);
    WOL.sendMagicPacket(appConfig.wolDevices[id].mac);
    return true;
}

inline String wolStatusJson() {
    String json = "[";
    for (int i = 0; i < appConfig.wolCount; i++) {
        if (i > 0) json += ",";
        json += "{\"id\":" + String(i);
        json += ",\"name\":\"" + String(appConfig.wolDevices[i].name) + "\"";
        json += ",\"mac\":\"" + String(appConfig.wolDevices[i].mac) + "\"";
        json += "}";
    }
    json += "]";
    return json;
}

#endif
