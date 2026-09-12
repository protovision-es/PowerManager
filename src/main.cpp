#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

#include "config.h"
#include "storage.h"
#include "relay.h"
#include "wol.h"
#include "scheduler.h"
#include "ping.h"
#include "log.h"
#include "webserver.h"

static bool ledBlinkState = false;
static unsigned long lastLedToggle = 0;

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

    // Otherwise, blink continuously as a heartbeat (program is alive).
    unsigned long now = millis();
    if (now - lastLedToggle >= STATUS_LED_BLINK_INTERVAL) {
        lastLedToggle = now;
        ledBlinkState = !ledBlinkState;
        digitalWrite(STATUS_LED_PIN, ledBlinkState ? HIGH : LOW);
    }
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

void connectWiFi() {
    Serial.printf("[WiFi] SSID: '%s'\n", WIFI_SSID);

    if (strlen(WIFI_SSID) == 0) {
        Serial.println("[WiFi] ERROR: SSID is empty in config.h!");
        return;
    }

    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
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
        delay(1000);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] Connected! IP: %s RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
        Serial.printf("[WiFi] FAILED! Final: %s (%d)\n", wifiStatusStr(WiFi.status()), WiFi.status());
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== PowerManager ===");

    statusLedInit();
    initConfig();

    connectWiFi();

    relayInit();
    wolInit();
    schedulerInit();
    pingInit();
    webserverInit();

    ArduinoOTA.setHostname("powermanager");
    ArduinoOTA.onStart([]() { Serial.println("[OTA] Start"); });
    ArduinoOTA.onEnd([]() { Serial.println("[OTA] End"); });
    ArduinoOTA.onProgress([](unsigned int p, unsigned int t) { Serial.printf("[OTA] Progress: %u%%\r", (p / (t / 100))); });
    ArduinoOTA.onError([](ota_error_t e) { Serial.printf("[OTA] Error [%u]\n", e); });
    ArduinoOTA.begin();

    Serial.println("=== Ready ===");
}

void loop() {
    unsigned long now = millis();

    static unsigned long lastScheduler = 0;
    if (now - lastScheduler >= 1000) {
        lastScheduler = now;
        schedulerUpdate();
        relayUpdate();
    }

    statusLedUpdate();
    ArduinoOTA.handle();
    pingUpdate();

    static unsigned long lastWifiCheck = 0;
    if (now - lastWifiCheck >= 30000) {
        lastWifiCheck = now;
        if (WiFi.status() != WL_CONNECTED) {
            Serial.printf("[WiFi] Reconnecting...\n");
            WiFi.disconnect();
            delay(100);
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        }
    }

    delay(10);
}
