#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <esp_system.h>

#include "config.h"
#include "storage.h"
#include "log.h"
#include "relay.h"
#include "wol.h"
#include "scheduler.h"
#include "ping.h"
#include "actions.h"
#include "webserver.h"

#define HOSTNAME "powermanager"

// Heartbeat: a short flash of STATUS_LED_ON_MS every STATUS_LED_BLINK_INTERVAL ms
#ifndef STATUS_LED_ON_MS
#define STATUS_LED_ON_MS 80
#endif

void statusLedInit() {
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);
}

void statusLedUpdate() {
    // While any relay is restarting, keep the LED solid ON.
    if (relayAnyRestarting()) {
        digitalWrite(STATUS_LED_PIN, HIGH);
        return;
    }

    // Otherwise, a short heartbeat flash (program is alive).
    unsigned long phase = millis() % (unsigned long)STATUS_LED_BLINK_INTERVAL;
    digitalWrite(STATUS_LED_PIN, phase < STATUS_LED_ON_MS ? HIGH : LOW);
}

const char* wifiStatusStr(wl_status_t s) {
    switch (s) {
        case WL_IDLE_STATUS:     return "IDLE";
        case WL_NO_SSID_AVAIL:   return "NO_SSID_AVAIL";
        case WL_SCAN_COMPLETED:  return "SCAN_COMPLETED";
        case WL_CONNECTED:       return "CONNECTED";
        case WL_CONNECT_FAILED:  return "CONNECT_FAILED";
        case WL_CONNECTION_LOST: return "CONNECTION_LOST";
        case WL_DISCONNECTED:    return "DISCONNECTED";
        default: return "UNKNOWN";
    }
}

const char* resetReasonStr() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   return "power on";
        case ESP_RST_SW:        return "software restart / OTA";
        case ESP_RST_PANIC:     return "crash (panic)";
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:       return "watchdog";
        case ESP_RST_BROWNOUT:  return "brownout (low voltage)";
        case ESP_RST_EXT:       return "reset button";
        case ESP_RST_DEEPSLEEP: return "deep sleep";
        default:                return "unknown";
    }
}

void connectWiFi() {
    Serial.printf("[WiFi] SSID: '%s'\n", WIFI_SSID);

    if (strlen(WIFI_SSID) == 0) {
        Serial.println("[WiFi] ERROR: SSID is empty in config.h!");
        return;
    }

    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    WiFi.setSleep(false);          // faster web responses and reliable WoL
    WiFi.setAutoReconnect(true);
    delay(100);

    if (strlen(WIFI_IP) > 0) {
        IPAddress ip, gateway, subnet, dns1, dns2;
        ip.fromString(WIFI_IP);
        gateway.fromString(WIFI_GATEWAY);
        subnet.fromString(WIFI_SUBNET);
        dns1.fromString(WIFI_DNS1);
        dns2.fromString(WIFI_DNS2);
        WiFi.config(ip, gateway, subnet, dns1, dns2);
        Serial.printf("[WiFi] Static IP: %s GW: %s\n", WIFI_IP, WIFI_GATEWAY);
    }

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT) {
        Serial.printf("[WiFi] Status: %s (%d)\n", wifiStatusStr(WiFi.status()), WiFi.status());
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] Connected! IP: %s RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
        Serial.printf("[WiFi] FAILED! Final: %s (%d). Will keep retrying.\n", wifiStatusStr(WiFi.status()), WiFi.status());
    }
}

// Logs WiFi drops/recoveries and nudges a reconnect if auto-reconnect stalls.
void wifiWatchdog() {
    static bool wasConnected = true;
    static unsigned long disconnectedSince = 0;
    static unsigned long lastRetry = 0;
    unsigned long now = millis();
    bool connected = WiFi.status() == WL_CONNECTED;

    if (connected) {
        if (!wasConnected) {
            logAddf("WiFi reconnected after %lus (RSSI %d dBm)",
                (now - disconnectedSince) / 1000UL, WiFi.RSSI());
        }
        wasConnected = true;
        return;
    }

    if (wasConnected) {
        wasConnected = false;
        disconnectedSince = now;
        lastRetry = now;
        logAdd("WiFi connection lost");
    }
    if (now - lastRetry >= 30000UL) {
        lastRetry = now;
        Serial.println("[WiFi] Reconnecting...");
        WiFi.reconnect();
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== PowerManager ===");

    // Relays first: until the pins are driven the relays are inactive, which
    // cuts power to the equipment. Doing this before the (up to 15 s) WiFi
    // connection keeps that blip to a fraction of a second on every reboot.
    initConfig();
    relayInit();

    statusLedInit();
    logInit();
    logAddf("Boot (%s)", resetReasonStr());

    connectWiFi();

    wolInit();
    schedulerInit();
    pingInit();
    webserverInit();

    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.onStart([]() { Serial.println("[OTA] Start"); });
    ArduinoOTA.onEnd([]() { Serial.println("[OTA] End"); });
    ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
        if (t) Serial.printf("[OTA] Progress: %u%%\r", (unsigned)((uint64_t)p * 100 / t));
    });
    ArduinoOTA.onError([](ota_error_t e) { Serial.printf("[OTA] Error [%u]\n", e); });
    ArduinoOTA.begin();
    MDNS.addService("http", "tcp", WEB_PORT);   // ArduinoOTA already started mDNS

    Serial.println("=== Ready ===");
}

void loop() {
    unsigned long now = millis();

    actionsProcess();     // actions requested from the web server
    relayUpdate();        // checked every loop for accurate restart durations

    static unsigned long lastScheduler = 0;
    if (now - lastScheduler >= 1000) {
        lastScheduler = now;
        schedulerUpdate();
    }

    statusLedUpdate();
    ArduinoOTA.handle();
    wifiWatchdog();

    delay(10);
}
