#ifndef STORAGE_H
#define STORAGE_H

#include "config.h"

struct RelayConfig {
    char name[32];
    int  pin;
    int  restartSec;
    bool restarting;
    unsigned long restartStart;
};

struct WoLDevice {
    char name[32];
    char mac[18];
};

struct ScheduleEntry {
    int  relay;
    int  hour;
    int  min;
    bool enabled;
    bool ranToday;
};

struct PingConfig {
    char host[64];
    int  intervalSec;
    int  failThreshold;
    int  relayTrigger;
};

struct AppConfig {
    RelayConfig    relays[RELAY_COUNT];
    WoLDevice      wolDevices[WOL_MAX_DEVICES];
    int            wolCount;
    ScheduleEntry  schedules[SCHEDULE_MAX];
    int            scheduleCount;
    PingConfig     ping;
    char           ntpServer[64];
    long           gmtOffset;
};

static AppConfig appConfig;

inline void initConfig() {
    // --- Relés ---
    int pins[] = {RELAY_0_PIN, RELAY_1_PIN, RELAY_2_PIN, RELAY_3_PIN};
    const char* names[] = {RELAY_0_NAME, RELAY_1_NAME, RELAY_2_NAME, RELAY_3_NAME};
    int restarts[] = {RELAY_0_RESTART_SEC, RELAY_1_RESTART_SEC, RELAY_2_RESTART_SEC, RELAY_3_RESTART_SEC};
    for (int i = 0; i < RELAY_COUNT; i++) {
        strlcpy(appConfig.relays[i].name, names[i], 32);
        appConfig.relays[i].pin = pins[i];
        appConfig.relays[i].restartSec = restarts[i];
        appConfig.relays[i].restarting = false;
        appConfig.relays[i].restartStart = 0;
    }

    // --- WoL ---
    struct { const char* name; const char* mac; } wolDef[] = {
        {WOL_0_NAME, WOL_0_MAC},
        {WOL_1_NAME, WOL_1_MAC},
        {WOL_2_NAME, WOL_2_MAC},
        {WOL_3_NAME, WOL_3_MAC},
        {WOL_4_NAME, WOL_4_MAC}
    };
    appConfig.wolCount = 0;
    for (int i = 0; i < WOL_MAX_DEVICES; i++) {
        if (strlen(wolDef[i].name) > 0 && strlen(wolDef[i].mac) > 0) {
            strlcpy(appConfig.wolDevices[appConfig.wolCount].name, wolDef[i].name, 32);
            strlcpy(appConfig.wolDevices[appConfig.wolCount].mac, wolDef[i].mac, 18);
            appConfig.wolCount++;
        }
    }

    // --- Horarios de reinicio ---
    struct { bool en; int r; int h; int m; } schedDef[] = {
        {SCHED_0_ENABLED, SCHED_0_RELAY, SCHED_0_HOUR, SCHED_0_MIN},
        {SCHED_1_ENABLED, SCHED_1_RELAY, SCHED_1_HOUR, SCHED_1_MIN},
        {SCHED_2_ENABLED, SCHED_2_RELAY, SCHED_2_HOUR, SCHED_2_MIN},
        {SCHED_3_ENABLED, SCHED_3_RELAY, SCHED_3_HOUR, SCHED_3_MIN},
        {SCHED_4_ENABLED, SCHED_4_RELAY, SCHED_4_HOUR, SCHED_4_MIN},
        {SCHED_5_ENABLED, SCHED_5_RELAY, SCHED_5_HOUR, SCHED_5_MIN},
        {SCHED_6_ENABLED, SCHED_6_RELAY, SCHED_6_HOUR, SCHED_6_MIN},
        {SCHED_7_ENABLED, SCHED_7_RELAY, SCHED_7_HOUR, SCHED_7_MIN},
        {SCHED_8_ENABLED, SCHED_8_RELAY, SCHED_8_HOUR, SCHED_8_MIN},
        {SCHED_9_ENABLED, SCHED_9_RELAY, SCHED_9_HOUR, SCHED_9_MIN}
    };
    appConfig.scheduleCount = 0;
    for (int i = 0; i < SCHEDULE_MAX; i++) {
        if (schedDef[i].en) {
            ScheduleEntry &s = appConfig.schedules[appConfig.scheduleCount];
            s.relay = schedDef[i].r;
            s.hour = schedDef[i].h;
            s.min = schedDef[i].m;
            s.enabled = true;
            s.ranToday = false;
            appConfig.scheduleCount++;
        }
    }

    // --- Ping ---
    strlcpy(appConfig.ping.host, PING_HOST, sizeof(appConfig.ping.host));
    appConfig.ping.intervalSec = PING_INTERVAL;
    appConfig.ping.failThreshold = PING_THRESHOLD;
    appConfig.ping.relayTrigger = PING_RELAY;

    // --- NTP ---
    strlcpy(appConfig.ntpServer, NTP_SERVER, sizeof(appConfig.ntpServer));
    appConfig.gmtOffset = NTP_GMT_OFFSET;

    Serial.printf("[Config] Relays: %d, WoL: %d, Schedules: %d\n",
        RELAY_COUNT, appConfig.wolCount, appConfig.scheduleCount);
}

#endif
