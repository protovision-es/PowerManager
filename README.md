# PowerManager

An ESP32-based power management system for remotely restarting network gear. It drives 4 relays to power-cycle equipment, sends Wake-on-LAN packets, runs daily restart schedules, and monitors your internet connection — all through a self-contained web dashboard with no external dependencies.

## Features

- **4 relays** — individually configurable restart (power-cycle) duration per relay
- **Wake on LAN** — power on remote machines by sending magic packets
- **Scheduled restarts** — up to 10 daily schedules for restarting relays
- **Ping monitor** — automatically restarts the router when the internet connection drops
- **Web dashboard** — responsive, auto-refreshing every 3 seconds, with a light/dark theme, live restart countdowns, connection-loss detection, and confirmation prompts before restarting a relay. Built with plain HTML/CSS/JavaScript — no frameworks, no build step, no CDN
- **API reference page** — a built-in `/api-doc` page listing every endpoint with ready-to-copy URLs
- **Bicolor status LED** — one color blinks to show the firmware is alive, the other lights up solid while any relay is mid-restart
- **WiFi** — static IP with automatic reconnection

## Hardware

### Relays

| Relay | GPIO | Default name | OFF duration |
|-------|------|---------------|---------------|
| 1 | 16 | Router | 10s |
| 2 | 17 | Server | 15s |
| 3 | 18 | NAS | 20s |
| 4 | 19 | Switch | 5s |

**Logic:** relays are ON by default. To restart a device, the relay turns off for X seconds and then turns back on.

- `RELAY_ACTIVE = HIGH` — logic level that activates the relay
- `RELAY_INACTIVE = LOW` — logic level that deactivates the relay

Adjust these two if your relay board is active-low.

### Bicolor status LED

| Element | GPIO (`config.h`) |
|---------|---------------------|
| "Cycle" color (blinks) | `STATUS_LED_CYCLE_PIN` (2) |
| "Restart" color (solid while a relay is restarting) | `STATUS_LED_RESTART_PIN` (4) |

Wiring: each color of the bicolor LED with its own resistor (330 ohm) between its GPIO and GND (or to a common 3V3 anode, depending on the LED type).

- The **cycle** color blinks continuously while the firmware is running (a heartbeat indicator).
- The **restart** color turns on solid as soon as **any** relay enters a restart cycle (off, waiting to turn back on) and turns off once every relay is back ON.

## Getting Started

### Requirements

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- An ESP32 dev board (tested with `esp32doit-devkit-v1`)
- 4-channel relay board, a bicolor LED, and whatever equipment you want to manage

### 1. Clone and configure

`include/config.h` holds your WiFi credentials and personal setup, so it's gitignored and never committed. Start from the template instead:

```bash
cp include/config.h.example include/config.h
```

Then edit `include/config.h` with your own WiFi network, relay names/pins, Wake-on-LAN devices, schedules, and ping target (see [Configuration](#configuration) below).

### 2. Build and upload

```bash
# Build
pio run

# First upload: over USB
# Uncomment "upload_port = COMx" (or /dev/ttyUSBx) in platformio.ini
# and set it to your board's serial port, then:
pio run --target upload

# Serial monitor
pio device monitor
```

After the first flash, the firmware advertises itself over mDNS as `powermanager.local` and OTA updates are enabled — `platformio.ini` is already set to upload over WiFi (`upload_protocol = espota`), so from then on `pio run --target upload` works without a USB cable, as long as your computer is on the same network.

### 3. Open the dashboard

Visit `http://<device-ip>/` (or `http://powermanager.local/`) in a browser.

## Configuration

All configuration happens at compile time in `include/config.h`. There's no filesystem storage (LittleFS) or runtime configuration UI.

### Status LED

```c
#define STATUS_LED_CYCLE_PIN      2      // Color 1: cycle blink
#define STATUS_LED_CYCLE_INTERVAL 100    // ms between each blink
#define STATUS_LED_RESTART_PIN    4      // Color 2: solid during a relay restart
```

### WiFi

```c
#define WIFI_SSID           "YourWiFiSSID"
#define WIFI_PASSWORD       "YourWiFiPassword"
#define WIFI_IP             "192.168.1.50"
#define WIFI_GATEWAY        "192.168.1.1"
#define WIFI_SUBNET         "255.255.255.0"
#define WIFI_DNS1           "8.8.8.8"
#define WIFI_DNS2           "1.1.1.1"
#define WIFI_CONNECT_TIMEOUT 15000
```

### Relays

```c
#define RELAY_COUNT         4
#define RELAY_ACTIVE        HIGH
#define RELAY_INACTIVE      LOW

#define RELAY_0_PIN         16
#define RELAY_0_NAME        "Router"
#define RELAY_0_RESTART_SEC 10
```

Repeat for `RELAY_1`, `RELAY_2`, `RELAY_3`.

### Wake on LAN

Up to 5 devices. Leave name and MAC empty for unused slots.

```c
#define WOL_0_NAME  "My Desktop PC"
#define WOL_0_MAC   "AA:BB:CC:DD:EE:FF"
```

### Scheduled restarts

Up to 10 entries. Each entry means "restart this relay at this time".

```c
#define SCHED_0_ENABLED  true
#define SCHED_0_RELAY    0       // Relay index (0-3)
#define SCHED_0_HOUR     14      // Hour (0-23)
#define SCHED_0_MIN      44      // Minute (0-59)
```

### Ping monitor

```c
#define PING_HOST       "8.8.8.8"
#define PING_INTERVAL   300     // Seconds between checks
#define PING_THRESHOLD  3       // Consecutive failures before restarting
#define PING_RELAY      0       // Which relay is the router
```

## API Reference

The server listens on port 80. `GET` endpoints returning `JSON` are meant for integrations; the ones marked `HTML` open a human-readable confirmation page and run the action as soon as the URL is requested — they work equally well opened in a browser, called from a shortcut/NFC tag, or hit with `curl`/`fetch`. A live, browsable version of this reference is also served at `/api-doc`.

### `GET /api/status` — JSON

Full system status: relays, WoL devices, ping, and the device clock.

**Response:**
```json
{
  "uptime": 3661,
  "datetime": "Mon 29 Jul 2026  14:30:00",
  "ip": "192.168.1.50",
  "wifi_rssi": -45,
  "relays": [
    {
      "id": 0,
      "name": "Router",
      "pin": 16,
      "restartSec": 10,
      "on": true,
      "restarting": false,
      "remaining": 0
    }
  ],
  "wol": [
    {
      "id": 0,
      "name": "My Desktop PC",
      "mac": "AA:BB:CC:DD:EE:FF"
    }
  ],
  "ping": {
    "host": "8.8.8.8",
    "interval": 300,
    "threshold": 3,
    "relayTrigger": 0,
    "failCount": 0,
    "lastResult": true,
    "ms": 24
  }
}
```

### `GET /api/relays/restart?id=X` — HTML

Restarts a relay (turns it off for X seconds, then back on) and returns a confirmation page with a live countdown. The restart is triggered as soon as the request is received, before the page even renders.

**Parameters:**
- `id` (query) — relay index (0-3)

An invalid `id` returns an HTML error page (HTTP 400) instead of triggering a restart.

### `GET /api/wol/wake?id=X` — HTML

Sends a Wake-on-LAN magic packet to a device and returns an HTML confirmation page.

**Parameters:**
- `id` (query) — device index (0-4)

An invalid `id` returns an HTML error page (HTTP 400).

### `GET /api/schedule` — JSON

The configured restart schedules and the device's current time.

**Response:**
```json
{
  "time": "14:30",
  "schedules": [
    {
      "id": 0,
      "relay": 0,
      "relayName": "Router",
      "hour": 14,
      "min": 44,
      "enabled": true
    }
  ]
}
```

### `GET /api/log` — JSON

The last 50 system events, oldest first.

**Response:**
```json
[
  {"t": "29/07 14:30:12", "m": "Relay Router restarting (OFF 10s)"},
  {"t": "29/07 14:30:22", "m": "Relay Router ON (restart complete)"},
  {"t": "29/07 14:31:00", "m": "WoL sent to My Desktop PC"},
  {"t": "29/07 14:35:00", "m": "Scheduled: Server at 05:00"},
  {"t": "29/07 14:40:00", "m": "Ping failed 3 times, restarting Router"}
]
```

### `GET /` — HTML

The web dashboard (embedded in flash via `PROGMEM`).

### `GET /api-doc` — HTML

The API reference page described above.

### Any other route

Falls back to the dashboard (SPA-style fallback via `onNotFound`).

## Project Structure

```
PowerManager/
├── include/
│   ├── config.h.example  # Configuration template (no personal data, tracked in git)
│   ├── config.h          # Your real configuration (WiFi, relays, WoL, schedules, ping) — gitignored
│   ├── storage.h         # Config structs and runtime config loading
│   ├── relay.h           # Relay control (init, restart, update, JSON)
│   ├── wol.h             # Wake on LAN (init, send, JSON)
│   ├── scheduler.h       # Daily restart scheduling
│   ├── ping.h            # Internet connectivity monitoring
│   └── webserver.h       # Web server + embedded dashboard/API-doc HTML + API handlers
├── src/
│   └── main.cpp          # setup()/loop(), WiFi connection, status LED
├── platformio.ini        # Build configuration
└── README.md
```

## Memory Usage

Approximate figures as of the last measured build — check with `pio run` for current numbers, especially after editing `webserver.h`, since most of the flash usage there is the embedded dashboard/API-doc HTML.

| Resource | Used | Total | Percentage |
|----------|------|-------|------------|
| RAM | ~52 KB | 320 KB | ~16% |
| Flash | ~1.1 MB | 1.3 MB | ~84% |
