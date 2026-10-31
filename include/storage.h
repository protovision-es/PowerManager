#ifndef STORAGE_H
#define STORAGE_H

#include "config.h"

// ---- Optional settings (older config.h files may not define them) ----

// POSIX time zone string, e.g. "CET-1CEST,M3.5.0,M10.5.0/3" for Spain.
// Handles daylight saving automatically. If empty, NTP_GMT_OFFSET is used.
#ifndef NTP_TZ
#define NTP_TZ ""
#endif

// Optional HTTP Basic Auth for the dashboard and API. Leave empty to disable.
#ifndef WEB_AUTH_USER
#define WEB_AUTH_USER ""
#endif
#ifndef WEB_AUTH_PASS
#define WEB_AUTH_PASS ""
#endif

// Minutes the ping monitor waits after it restarts the router before it
// starts counting failures again (the router needs time to boot).
// Minutes without internet before the router relay is restarted.
// (Older config.h files used PING_INTERVAL * PING_THRESHOLD seconds instead.)
#ifndef PING_INTERVAL
#define PING_INTERVAL 300
#endif
#ifndef PING_THRESHOLD
#define PING_THRESHOLD 3
#endif
#if defined(PING_RESTART_AFTER_MIN) && PING_RESTART_AFTER_MIN < 1
#error "PING_RESTART_AFTER_MIN must be >= 1"
#endif

#ifndef PING_GRACE_MIN
#define PING_GRACE_MIN 5
#endif

// Seconds between internet checks. They are grouped per minute in the
// history, so it must divide 60 (10, 12, 15, 20, 30 or 60).
#ifndef PING_PROBE_SEC
#define PING_PROBE_SEC 15
#endif
#if PING_PROBE_SEC < 10 || PING_PROBE_SEC > 60 || (60 % PING_PROBE_SEC) != 0
#error "PING_PROBE_SEC must divide 60 (10, 12, 15, 20, 30 or 60)"
#endif

struct RelayConfig {
    char name[32];
    int  pin;
    int  restartSec;
    bool restarting;
    unsigned long restartStart;   // millis() when the current restart began
    // stats since boot
    uint16_t restartCount;
    unsigned long lastRestart;    // millis() of the last restart, 0 = never
    const char *lastReason;       // "web", "schedule", "ping", "api", ...
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
    int  lastRunYday;             // tm_yday of the last run, -1 = never
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
    char           tz[48];
    long           gmtOffset;
};

static AppConfig appConfig;

inline void initConfig() {
    // --- Relays ---
    int pins[] = {RELAY_0_PIN, RELAY_1_PIN, RELAY_2_PIN, RELAY_3_PIN};
    const char* names[] = {RELAY_0_NAME, RELAY_1_NAME, RELAY_2_NAME, RELAY_3_NAME};
    int restarts[] = {RELAY_0_RESTART_SEC, RELAY_1_RESTART_SEC, RELAY_2_RESTART_SEC, RELAY_3_RESTART_SEC};
    for (int i = 0; i < RELAY_COUNT; i++) {
        strlcpy(appConfig.relays[i].name, names[i], sizeof(appConfig.relays[i].name));
        appConfig.relays[i].pin = pins[i];
        appConfig.relays[i].restartSec = restarts[i];
        appConfig.relays[i].restarting = false;
        appConfig.relays[i].restartStart = 0;
        appConfig.relays[i].restartCount = 0;
        appConfig.relays[i].lastRestart = 0;
        appConfig.relays[i].lastReason = "";
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

    // --- Scheduled restarts ---
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
        if (!schedDef[i].en) continue;
        // Skip invalid entries instead of indexing out of bounds later
        if (schedDef[i].r < 0 || schedDef[i].r >= RELAY_COUNT ||
            schedDef[i].h < 0 || schedDef[i].h > 23 ||
            schedDef[i].m < 0 || schedDef[i].m > 59) {
            Serial.printf("[Config] Schedule %d ignored (invalid relay/time)\n", i);
            continue;
        }
        ScheduleEntry &s = appConfig.schedules[appConfig.scheduleCount];
        s.relay = schedDef[i].r;
        s.hour = schedDef[i].h;
        s.min = schedDef[i].m;
        s.enabled = true;
        s.lastRunYday = -1;
        appConfig.scheduleCount++;
    }

    // --- Ping ---
    strlcpy(appConfig.ping.host, PING_HOST, sizeof(appConfig.ping.host));
    appConfig.ping.intervalSec = PING_INTERVAL;
    appConfig.ping.failThreshold = PING_THRESHOLD;
    appConfig.ping.relayTrigger = (PING_RELAY >= 0 && PING_RELAY < RELAY_COUNT) ? PING_RELAY : 0;

    // --- NTP ---
    strlcpy(appConfig.ntpServer, NTP_SERVER, sizeof(appConfig.ntpServer));
    strlcpy(appConfig.tz, NTP_TZ, sizeof(appConfig.tz));
    appConfig.gmtOffset = NTP_GMT_OFFSET;

    Serial.printf("[Config] Relays: %d, WoL: %d, Schedules: %d\n",
        RELAY_COUNT, appConfig.wolCount, appConfig.scheduleCount);
}

#endif
