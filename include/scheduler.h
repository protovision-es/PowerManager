#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <time.h>
#include "config.h"
#include "storage.h"
#include "relay.h"
#include "log.h"

inline void schedulerInit() {
    configTime(appConfig.gmtOffset * 3600, 0, appConfig.ntpServer);
    Serial.println("[Scheduler] NTP synced");
}

inline void schedulerUpdate() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return;

    int nowH = timeinfo.tm_hour;
    int nowM = timeinfo.tm_min;

    for (int i = 0; i < appConfig.scheduleCount; i++) {
        ScheduleEntry &s = appConfig.schedules[i];
        if (!s.enabled) continue;

        if (nowH == 0 && nowM == 0) {
            s.ranToday = false;
        }

        if (!s.ranToday && nowH == s.hour && nowM == s.min) {
            relayRestart(s.relay);
            s.ranToday = true;
            char buf[64];
            snprintf(buf, sizeof(buf), "Scheduled: %s at %02d:%02d", appConfig.relays[s.relay].name, s.hour, s.min);
            logAdd(buf);
            Serial.printf("[Scheduler] Scheduled restart: %s at %02d:%02d\n",
                appConfig.relays[s.relay].name, s.hour, s.min);
        }
    }
}

inline String scheduleStatusJson() {
    struct tm timeinfo;
    String currentTime = "??:??";
    if (getLocalTime(&timeinfo)) {
        char buf[6];
        snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        currentTime = buf;
    }

    String json = "{\"time\":\"" + currentTime + "\",\"schedules\":[";
    for (int i = 0; i < appConfig.scheduleCount; i++) {
        if (i > 0) json += ",";
        ScheduleEntry &s = appConfig.schedules[i];
        json += "{\"id\":" + String(i);
        json += ",\"relay\":" + String(s.relay);
        json += ",\"relayName\":\"" + String(appConfig.relays[s.relay].name) + "\"";
        json += ",\"hour\":" + String(s.hour);
        json += ",\"min\":" + String(s.min);
        json += ",\"enabled\":" + String(s.enabled ? "true" : "false");
        json += "}";
    }
    json += "]}";
    return json;
}

#endif
