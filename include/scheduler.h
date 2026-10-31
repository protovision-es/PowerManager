#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <time.h>
#include "config.h"
#include "storage.h"
#include "relay.h"
#include "log.h"
#include "util.h"

static bool schedClockSynced = false;

inline void schedulerInit() {
    if (strlen(appConfig.tz) > 0) {
        // POSIX TZ string: local time with automatic daylight saving
        configTzTime(appConfig.tz, appConfig.ntpServer);
        Serial.printf("[Scheduler] NTP %s, TZ %s\n", appConfig.ntpServer, appConfig.tz);
    } else {
        configTime(appConfig.gmtOffset * 3600, 0, appConfig.ntpServer);
        Serial.printf("[Scheduler] NTP %s, GMT%+ld (no DST, set NTP_TZ)\n",
            appConfig.ntpServer, appConfig.gmtOffset);
    }
}

// Called once per second from loop().
inline void schedulerUpdate() {
    struct tm t;
    if (!timeNow(t)) return;   // clock not synced yet: schedules wait

    if (!schedClockSynced) {
        schedClockSynced = true;
        char buf[24];
        strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M", &t);
        logAddf("Clock synced (NTP): %s", buf);
    }

    for (int i = 0; i < appConfig.scheduleCount; i++) {
        ScheduleEntry &s = appConfig.schedules[i];
        if (!s.enabled) continue;
        // lastRunYday guarantees one run per day, even though this runs
        // 60 times during the matching minute.
        if (t.tm_hour == s.hour && t.tm_min == s.min && s.lastRunYday != t.tm_yday) {
            s.lastRunYday = t.tm_yday;
            if (!relayRestart(s.relay, "schedule")) {
                logAddf("Schedule %02d:%02d skipped: %s already restarting",
                    s.hour, s.min, appConfig.relays[s.relay].name);
            }
        }
    }
}

inline String scheduleStatusJson() {
    struct tm t;
    bool synced = timeNow(t);
    char now[6] = "??:??";
    if (synced) snprintf(now, sizeof(now), "%02d:%02d", t.tm_hour, t.tm_min);

    String json = "{\"time\":\"";
    json += now;
    json += "\",\"synced\":";
    json += synced ? "true" : "false";
    json += ",\"schedules\":[";
    for (int i = 0; i < appConfig.scheduleCount; i++) {
        if (i > 0) json += ",";
        const ScheduleEntry &s = appConfig.schedules[i];
        json += "{\"id\":" + String(i);
        json += ",\"relay\":" + String(s.relay);
        json += ",\"relayName\":\"" + jsonEscape(appConfig.relays[s.relay].name) + "\"";
        json += ",\"hour\":" + String(s.hour);
        json += ",\"min\":" + String(s.min);
        json += ",\"enabled\":" + String(s.enabled ? "true" : "false");
        json += ",\"ranToday\":" + String((synced && s.lastRunYday == t.tm_yday) ? "true" : "false");
        json += "}";
    }
    json += "]}";
    return json;
}

#endif
