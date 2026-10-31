#ifndef PING_H
#define PING_H

// ============================================================
// Internet monitor
//
// A dedicated FreeRTOS task checks the connection every PING_PROBE_SEC
// (TCP connect to PING_HOST:PING_PORT; ICMP needs raw sockets) and keeps a
// minute-by-minute history of the last 7 days on the device itself, so it is
// recorded whether or not anyone has the dashboard open.
//
// Per minute (3 bytes): average latency, checks done / failed, "no WiFi"
// flag, average WiFi RSSI. The week (10080 minutes, ~31 KB) lives in RAM
// and is saved to the LittleFS partition every 10 minutes and after every
// outage, so it survives reboots and firmware updates.
//
// If the connection stays down for PING_RESTART_AFTER_MIN minutes the router
// relay is restarted, then restarts pause for PING_GRACE_MIN.
// ============================================================

#include <WiFi.h>
#include <WiFiClient.h>
#include <LittleFS.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "config.h"
#include "storage.h"
#include "log.h"
#include "util.h"
#include "actionq.h"

#ifndef PING_PORT
#define PING_PORT 53
#endif
#define PING_TIMEOUT_MS      1500       // per attempt; a check = up to 2 attempts

#define NET_MINUTES          10080      // 7 days
#define NET_HOURS            168
#define NET_OUTAGES          40
#define NET_PRE_LEN          960        // checks buffered before the clock is synced (4 h at 15 s)
#define NET_SAVE_EVERY_MS    (10UL * 60UL * 1000UL)
#define NET_FIRST_PROBE_MS   8000UL
#define NET_MIN_OUTAGE       2          // consecutive failed checks to count as an outage
#define NET_FILE             "/nethist.bin"
#define NET_FILE_TMP         "/nethist.tmp"
#define NET_FILE_MAGIC       0x324E4D50UL   // "PMN2"

// Check result codes: >= 0 latency in ms
#define NET_FAIL    -1                  // WiFi up, host unreachable
#define NET_NOWIFI  -2                  // ESP32 not connected to WiFi

// One minute of history. flags: bits 0-2 checks done, bits 3-5 failed,
// bit 6 = at least one check without WiFi. All zero = no data.
struct MinuteRec {
    uint8_t lat;        // encoded average latency of the successful checks (0 = none)
    uint8_t flags;
    int8_t  rssi;       // average dBm, 0 = unknown
};

struct Outage {         // 12 bytes
    uint32_t start;     // epoch, 0 = unknown (clock not synced)
    uint32_t dur;       // seconds
    uint8_t  restarted; // the router relay was restarted during it
    uint8_t  noWifi;    // the ESP32 had lost WiFi (vs. no internet)
    uint16_t reserved;
};

struct PreProbe {       // check done before the clock was synced
    uint32_t tick;      // millis()
    int16_t  code;
    int8_t   rssi;
};

// ---- shared state (protected by netMutex) ----
static SemaphoreHandle_t netMutex = nullptr;
static TaskHandle_t netTask = nullptr;
static bool netFsOk = false;

static MinuteRec netMin[NET_MINUTES];          // slot = (hour % 168) * 60 + minute
static uint32_t netHourStamp[NET_HOURS];       // epoch hour stored in each 60-minute block
static Outage netOutages[NET_OUTAGES];
static uint8_t netOutHead = 0, netOutCount = 0;
static PreProbe netPre[NET_PRE_LEN];
static uint16_t netPreCount = 0;

// accumulators of the minute being recorded
static uint32_t netCurMin = 0;
static uint32_t netCurSum = 0;
static uint8_t netCurOk = 0, netCurTotal = 0, netCurFail = 0;
static bool netCurNoWifi = false;
static int32_t netCurRssiSum = 0;
static uint8_t netCurRssiN = 0;
static uint32_t netLastMin = 0;                // newest minute with data (epoch minute)

// current state
static uint32_t netSeq = 0;                    // +1 per check
static bool pingChecked = false;
static bool lastPingResult = true;
static int lastPingMs = 0;
static unsigned long lastPingTime = 0;         // millis() of the last check
static int pingFailCount = 0;                  // consecutive failed checks
static bool netDown = false;
static unsigned long netDownStartMs = 0;
static unsigned long netDecisionFrom = 0;
static bool netDownRestarted = false;
static bool netDownNoWifi = false;
static unsigned long pingGraceUntil = 0;
static volatile bool pingNowRequested = false;
static bool netDirty = false;

inline uint32_t netRestartAfterSec() {
#ifdef PING_RESTART_AFTER_MIN
    return (uint32_t)PING_RESTART_AFTER_MIN * 60UL;
#else
    // older config.h files: PING_INTERVAL * PING_THRESHOLD seconds
    return (uint32_t)appConfig.ping.intervalSec * appConfig.ping.failThreshold;
#endif
}

// Latency <-> byte: 2 ms steps up to 298 ms, then 25 ms steps up to ~2.9 s.
inline uint8_t netEncLat(uint32_t ms) {
    if (ms < 300) return (uint8_t)(1 + ms / 2);
    uint32_t c = 151 + (ms - 300) / 25;
    return (uint8_t)(c > 255 ? 255 : c);
}

inline uint32_t netSlot(uint32_t epochMin) {
    return ((epochMin / 60) % NET_HOURS) * 60 + epochMin % 60;
}

// ---------------- persistence (LittleFS) ----------------

static void netLoad() {
    const char *path = LittleFS.exists(NET_FILE) ? NET_FILE : (LittleFS.exists(NET_FILE_TMP) ? NET_FILE_TMP : nullptr);
    if (!path) return;
    File f = LittleFS.open(path, "r");
    if (!f) return;
    uint32_t magic = 0;
    bool ok = f.read((uint8_t *)&magic, 4) == 4 && magic == NET_FILE_MAGIC &&
              f.read((uint8_t *)netHourStamp, sizeof(netHourStamp)) == sizeof(netHourStamp) &&
              f.read((uint8_t *)netMin, sizeof(netMin)) == sizeof(netMin) &&
              f.read((uint8_t *)netOutages, sizeof(netOutages)) == sizeof(netOutages) &&
              f.read(&netOutHead, 1) == 1 && f.read(&netOutCount, 1) == 1;
    f.close();
    if (!ok) {
        memset(netHourStamp, 0, sizeof(netHourStamp));
        memset(netMin, 0, sizeof(netMin));
        memset(netOutages, 0, sizeof(netOutages));
        netOutHead = netOutCount = 0;
        Serial.println("[Net] History file invalid, starting empty");
        return;
    }
    netOutHead %= NET_OUTAGES;
    if (netOutCount > NET_OUTAGES) netOutCount = NET_OUTAGES;
    // newest minute that has data
    for (uint32_t hi = 0; hi < NET_HOURS; hi++) {
        if (!netHourStamp[hi]) continue;
        for (int m = 59; m >= 0; m--) {
            if (netMin[hi * 60 + m].flags) {
                uint32_t em = netHourStamp[hi] * 60 + m;
                if (em > netLastMin) netLastMin = em;
                break;
            }
        }
    }
    Serial.println("[Net] History loaded from flash");
}

// Only the monitor task modifies the history, so it can save without the mutex.
static void netSave() {
    if (!netFsOk) return;
    File f = LittleFS.open(NET_FILE_TMP, "w");
    if (!f) return;
    uint32_t magic = NET_FILE_MAGIC;
    bool ok = f.write((uint8_t *)&magic, 4) == 4 &&
              f.write((uint8_t *)netHourStamp, sizeof(netHourStamp)) == sizeof(netHourStamp) &&
              f.write((uint8_t *)netMin, sizeof(netMin)) == sizeof(netMin) &&
              f.write((uint8_t *)netOutages, sizeof(netOutages)) == sizeof(netOutages) &&
              f.write(&netOutHead, 1) == 1 && f.write(&netOutCount, 1) == 1;
    f.close();
    if (!ok) { Serial.println("[Net] Save failed"); return; }
    LittleFS.remove(NET_FILE);
    LittleFS.rename(NET_FILE_TMP, NET_FILE);
    netDirty = false;
}

// ---------------- measurement ----------------

static int16_t netCheck(int &rssi) {
    rssi = 0;
    if (WiFi.status() != WL_CONNECTED) return NET_NOWIFI;
    rssi = WiFi.RSSI();

    IPAddress ip;
    if (!ip.fromString(appConfig.ping.host) && !WiFi.hostByName(appConfig.ping.host, ip)) return NET_FAIL;

    for (int i = 0; i < 2; i++) {        // a single lost packet should not count as a failure
        unsigned long start = millis();
        WiFiClient client;
        if (client.connect(ip, PING_PORT, PING_TIMEOUT_MS)) {
            unsigned long ms = millis() - start;
            client.stop();
            return (int16_t)min(ms, 30000UL);
        }
        client.stop();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    return NET_FAIL;
}

static void fmtDuration(char *buf, size_t len, uint32_t s) {
    if (s < 60) snprintf(buf, len, "%lus", (unsigned long)s);
    else if (s < 3600) snprintf(buf, len, "%lum %lus", (unsigned long)(s / 60), (unsigned long)(s % 60));
    else snprintf(buf, len, "%luh %lum", (unsigned long)(s / 3600), (unsigned long)((s % 3600) / 60));
}

// Adds one check to the minute it belongs to. Called with netMutex held.
static void netAddToMinute(uint32_t epochMin, int16_t code, int rssi) {
    uint32_t hour = epochMin / 60;
    uint32_t hi = hour % NET_HOURS;
    if (netHourStamp[hi] != hour) {              // block holds an older week: reuse it
        netHourStamp[hi] = hour;
        memset(&netMin[hi * 60], 0, 60 * sizeof(MinuteRec));
    }
    if (epochMin != netCurMin) {
        netCurMin = epochMin;
        netCurSum = 0; netCurOk = netCurTotal = netCurFail = 0;
        netCurNoWifi = false; netCurRssiSum = 0; netCurRssiN = 0;
    }
    if (netCurTotal < 7) netCurTotal++;
    if (code >= 0) { netCurSum += code; netCurOk++; }
    else if (netCurFail < 7) netCurFail++;
    if (code == NET_NOWIFI) netCurNoWifi = true;
    if (rssi < 0) { netCurRssiSum += rssi; netCurRssiN++; }

    MinuteRec &r = netMin[netSlot(epochMin)];
    r.lat = netCurOk ? netEncLat(netCurSum / netCurOk) : 0;
    r.flags = (netCurTotal & 7) | ((netCurFail & 7) << 3) | (netCurNoWifi ? 0x40 : 0);
    r.rssi = netCurRssiN ? (int8_t)(netCurRssiSum / netCurRssiN) : 0;
    if (epochMin > netLastMin) netLastMin = epochMin;
    netDirty = true;
}

static void netStoreOutage(uint32_t dur) {
    time_t now = time(nullptr);
    Outage &o = netOutages[netOutHead];
    o = {timeSynced() ? (uint32_t)(now - dur) : 0, dur, (uint8_t)netDownRestarted, (uint8_t)netDownNoWifi, 0};
    netOutHead = (netOutHead + 1) % NET_OUTAGES;
    if (netOutCount < NET_OUTAGES) netOutCount++;
}

// Records a scheduled check and runs the outage / restart logic. netMutex held.
static void netRecord(int16_t code, int rssi) {
    unsigned long now = millis();
    bool ok = code >= 0;
    netSeq++;
    pingChecked = true;
    lastPingResult = ok;
    lastPingMs = ok ? code : 0;
    lastPingTime = now ? now : 1;

    if (timeSynced()) {
        time_t epoch = time(nullptr);
        // replay checks done before the clock was known, now that we can date them
        for (uint16_t i = 0; i < netPreCount; i++) {
            uint32_t age = (now - netPre[i].tick) / 1000UL;
            netAddToMinute((uint32_t)((epoch - age) / 60), netPre[i].code, netPre[i].rssi);
        }
        netPreCount = 0;
        netAddToMinute((uint32_t)(epoch / 60), code, rssi);
    } else if (netPreCount < NET_PRE_LEN) {
        netPre[netPreCount++] = {(uint32_t)now, code, (int8_t)rssi};
    }

    if (ok) {
        if (netDown) {
            uint32_t dur = (now - netDownStartMs) / 1000UL;
            netStoreOutage(dur);
            char d[16];
            fmtDuration(d, sizeof(d), dur);
            logAddf("Internet back after %s (%d ms)", d, code);
            netDirty = true;
            netSave();
        }
        netDown = false;
        netDownRestarted = false;
        netDownNoWifi = false;
        pingFailCount = 0;
        return;
    }

    pingFailCount++;
    if (code == NET_NOWIFI) netDownNoWifi = true;
    if (pingFailCount == 1) {
        netDownStartMs = now;
        netDecisionFrom = now;
        if (pingGraceUntil && (long)(pingGraceUntil - now) > 0) netDecisionFrom = pingGraceUntil;
    }
    if (pingFailCount == NET_MIN_OUTAGE && !netDown) {
        netDown = true;
        logAddf("Internet down (%s)", code == NET_NOWIFI ? "no WiFi" : appConfig.ping.host);
    }

    bool inGrace = pingGraceUntil && (long)(pingGraceUntil - now) > 0;
    if (!inGrace && (long)(now - netDecisionFrom) >= (long)netRestartAfterSec() * 1000L) {
        int r = appConfig.ping.relayTrigger;
        char d[16];
        fmtDuration(d, sizeof(d), (now - netDownStartMs) / 1000UL);
        logAddf("No internet for %s, restarting %s", d, appConfig.relays[r].name);
        actionPush(ACTION_RESTART, r, "ping");
        netDownRestarted = true;
        pingGraceUntil = now + (unsigned long)PING_GRACE_MIN * 60000UL;
        if (pingGraceUntil == 0) pingGraceUntil = 1;
        netDecisionFrom = pingGraceUntil;
    }
}

static void netTaskFn(void *) {
    TickType_t period = pdMS_TO_TICKS(PING_PROBE_SEC * 1000UL);
    TickType_t nextDue = xTaskGetTickCount() + pdMS_TO_TICKS(NET_FIRST_PROBE_MS);
    unsigned long lastSave = millis();

    for (;;) {
        TickType_t nowT = xTaskGetTickCount();
        TickType_t wait = (int32_t)(nextDue - nowT) > 0 ? nextDue - nowT : 0;
        bool manual = ulTaskNotifyTake(pdTRUE, wait) > 0;

        int rssi = 0;
        int16_t code = netCheck(rssi);

        xSemaphoreTake(netMutex, portMAX_DELAY);
        if (manual) {
            // Manual checks report only: not stored, never trigger a restart
            pingChecked = true;
            lastPingResult = code >= 0;
            lastPingMs = code >= 0 ? code : 0;
            lastPingTime = millis() ? millis() : 1;
            pingNowRequested = false;
            xSemaphoreGive(netMutex);
            if (code >= 0) logAddf("Manual check: OK (%d ms)", code);
            else logAddf("Manual check: %s", code == NET_NOWIFI ? "no WiFi" : "no response");
            continue;
        }
        netRecord(code, rssi);
        xSemaphoreGive(netMutex);

        if (netDirty && millis() - lastSave >= NET_SAVE_EVERY_MS) {
            netSave();
            lastSave = millis();
        }

        nextDue += period;
        if ((int32_t)(xTaskGetTickCount() - nextDue) > 0) nextDue = xTaskGetTickCount() + period;
    }
}

// ---------------- public API ----------------

inline void pingInit() {
    if (!netMutex) netMutex = xSemaphoreCreateMutex();
    memset(netHourStamp, 0, sizeof(netHourStamp));
    memset(netMin, 0, sizeof(netMin));
    memset(netOutages, 0, sizeof(netOutages));
    netFsOk = LittleFS.begin(true);   // formats the (unused) data partition the first time
    if (netFsOk) netLoad();
    else logAdd("No LittleFS partition: internet history will not survive reboots");
    if (!netTask) xTaskCreatePinnedToCore(netTaskFn, "netmon", 6144, nullptr, 1, &netTask, 0);
    Serial.printf("[Net] %s:%d every %ds, restart after %lus down\n",
        appConfig.ping.host, PING_PORT, PING_PROBE_SEC, (unsigned long)netRestartAfterSec());
}

// Request an immediate check (safe from any task).
inline void pingRequestNow() {
    pingNowRequested = true;
    if (netTask) xTaskNotifyGive(netTask);
}

inline long pingGraceRemaining() {
    if (!pingGraceUntil) return 0;
    long left = (long)(pingGraceUntil - millis());
    return left > 0 ? (left + 999) / 1000 : 0;
}

inline String pingStatusJson() {
    xSemaphoreTake(netMutex, portMAX_DELAY);
    unsigned long now = millis();
    long nextIn = lastPingTime
        ? (long)PING_PROBE_SEC - secondsSince(lastPingTime)
        : (long)(NET_FIRST_PROBE_MS / 1000UL) - (long)(now / 1000UL);
    long downFor = pingFailCount > 0 ? (long)((now - netDownStartMs) / 1000UL) : 0;
    long restartIn = -1;
    if (pingFailCount > 0) {
        long left = (long)netRestartAfterSec() - (long)((long)(now - netDecisionFrom) / 1000L);
        restartIn = left > 0 ? left : 0;
    }
    String json = "{";
    json += "\"host\":\"" + jsonEscape(appConfig.ping.host) + "\"";
    json += ",\"port\":" + String(PING_PORT);
    json += ",\"probeSec\":" + String(PING_PROBE_SEC);
    json += ",\"restartAfter\":" + String(netRestartAfterSec());
    json += ",\"relayTrigger\":" + String(appConfig.ping.relayTrigger);
    json += ",\"checked\":" + String(pingChecked ? "true" : "false");
    json += ",\"lastResult\":" + String(lastPingResult ? "true" : "false");
    json += ",\"ms\":" + String(lastPingMs);
    json += ",\"lastAgo\":" + String(secondsSince(lastPingTime));
    json += ",\"nextIn\":" + String(nextIn > 0 ? nextIn : 0);
    json += ",\"failCount\":" + String(pingFailCount);
    json += ",\"down\":" + String(netDown ? "true" : "false");
    json += ",\"noWifi\":" + String(netDownNoWifi ? "true" : "false");
    json += ",\"downFor\":" + String(downFor);
    json += ",\"restartIn\":" + String(restartIn);
    json += ",\"grace\":" + String(pingGraceRemaining());
    json += ",\"pending\":" + String(pingNowRequested ? "true" : "false");
    json += ",\"seq\":" + String(netSeq);
    json += ",\"lastMin\":" + String(netLastMin);
    json += ",\"stored\":" + String(netFsOk ? "true" : "false");
    json += "}";
    xSemaphoreGive(netMutex);
    return json;
}

inline String pingOutagesJson() {
    xSemaphoreTake(netMutex, portMAX_DELAY);
    String json = "[";
    for (uint8_t i = 0; i < netOutCount; i++) {   // newest first
        const Outage &o = netOutages[(netOutHead + NET_OUTAGES - 1 - i) % NET_OUTAGES];
        if (i) json += ',';
        json += "[" + String(o.start) + "," + String(o.dur) + "," + String(o.restarted) + "," + String(o.noWifi) + "]";
    }
    json += "]";
    xSemaphoreGive(netMutex);
    return json;
}

// ---- binary history: 20-byte header + 3 bytes per minute ----
//   0  "PMH2"           4  version (2)     5  probe seconds   6  reserved (2)
//   8  now (epoch, u32) 12 first minute (epoch minute, u32)   16 count (u32)
//   then count x {lat, flags, rssi} for minutes first .. first+count-1
#define NET_HDR_LEN 20

inline uint32_t netHistoryRange(uint32_t from, uint32_t &first) {
    if (!timeSynced() || netLastMin == 0) { first = 0; return 0; }
    uint32_t nowMin = (uint32_t)(time(nullptr) / 60);
    uint32_t last = max(nowMin, netLastMin);
    uint32_t oldest = last - (NET_MINUTES - 1);
    first = (from > oldest && from <= last) ? from : oldest;
    return last - first + 1;
}

// Fills buf with bytes [index, index+maxLen) of the binary history.
inline size_t netHistoryFill(const uint8_t *hdr, uint32_t first, uint32_t count,
                             uint8_t *buf, size_t maxLen, size_t index) {
    size_t total = NET_HDR_LEN + (size_t)count * 3, n = 0;
    xSemaphoreTake(netMutex, portMAX_DELAY);
    while (n < maxLen && index + n < total) {
        size_t pos = index + n;
        if (pos < NET_HDR_LEN) { buf[n++] = hdr[pos]; continue; }
        size_t r = (pos - NET_HDR_LEN) / 3, b = (pos - NET_HDR_LEN) % 3;
        uint32_t m = first + r;
        uint32_t hour = m / 60;
        const MinuteRec &rec = netMin[netSlot(m)];
        bool valid = netHourStamp[hour % NET_HOURS] == hour;
        buf[n++] = !valid ? 0 : (b == 0 ? rec.lat : b == 1 ? rec.flags : (uint8_t)rec.rssi);
    }
    xSemaphoreGive(netMutex);
    return n;
}

#endif
