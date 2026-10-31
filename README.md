# PowerManager

An ESP32-based power management system for remotely restarting network gear. It drives 4 relays to power-cycle equipment, sends Wake-on-LAN packets, runs daily restart schedules, and monitors your internet connection — all through a self-contained web dashboard with no external dependencies.

## Features

- **4 relays** — individually configurable restart (power-cycle) duration per relay
- **Wake on LAN** — power on remote machines by sending magic packets
- **Scheduled restarts** — up to 10 daily schedules for restarting relays
- **Internet monitor** — the device checks the connection every 15 s on its own (no browser needed) and keeps a minute-by-minute history of the last 7 days in flash, so it survives reboots and updates. Pick any moment of the week and the dashboard tells you whether the internet was working then (no internet, unstable, ESP32 without WiFi, or no data). Also: uptime, latency (avg, p95, jitter), WiFi signal, a navigable latency chart (1 h / 6 h / 24 h / 7 days), a 7×24 hourly heatmap, the latency distribution and the outage list. It restarts the router after 15 min without internet, then pauses while it boots
- **Web dashboard** — responsive, auto-refreshing every 3 seconds, with a light/dark theme, live restart countdowns, last restart per relay (when and why), time until the next scheduled restart, connection-loss detection, and confirmation prompts before restarting a relay. Built with plain HTML/CSS/JavaScript — no frameworks, no build step, no CDN
- **JSON API** — `POST` endpoints for integrations (Home Assistant, scripts) plus plain `GET` links for shortcuts and NFC tags
- **Optional login** — HTTP Basic Auth for the dashboard and API
- **API reference page** — a built-in `/api-doc` page listing every endpoint with ready-to-copy URLs
- **Status LED** — a short heartbeat flash every second, and switches to solid ON while any relay is mid-restart
- **WiFi** — static IP, automatic reconnection, drops and recoveries recorded in the log
- **Local time** — NTP with a POSIX time zone, so schedules follow daylight saving

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

> **Note on reboots:** with this logic the equipment is powered only while the ESP32 drives the relay. The firmware sets the relay pins first thing at boot, so an ESP32 reboot (or OTA update) cuts power for well under a second, but it is not zero. If that matters, wire the equipment to the relay's **NC** contact and swap `RELAY_ACTIVE`/`RELAY_INACTIVE`: then a dead or rebooting ESP32 leaves everything powered.

### Status LED

| Element | GPIO (`config.h`) |
|---------|---------------------|
| Status LED | `STATUS_LED_PIN` (2) |

Wiring: a single LED with its own resistor (330 ohm) between `STATUS_LED_PIN` and GND.

- Normally, the LED gives a **short flash** (`STATUS_LED_ON_MS`) every `STATUS_LED_BLINK_INTERVAL` ms while the firmware is running (a heartbeat indicator).
- As soon as **any** relay enters a restart cycle (off, waiting to turn back on), the LED switches to **solid ON**, and goes back to blinking once every relay is back ON.

## Getting Started

### Requirements

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- An ESP32 dev board (tested with `esp32doit-devkit-v1`)
- 4-channel relay board, a status LED, and whatever equipment you want to manage

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
#define STATUS_LED_PIN            2      // GPIO for the status LED
#define STATUS_LED_BLINK_INTERVAL 1000   // ms between heartbeat flashes (1 per second)
#define STATUS_LED_ON_MS          80     // duration of each flash (ms)
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

### Time zone

```c
#define NTP_SERVER          "pool.ntp.org"
#define NTP_TZ              "CET-1CEST,M3.5.0,M10.5.0/3"   // POSIX TZ, handles DST
#define NTP_GMT_OFFSET      0                               // only used if NTP_TZ is ""
```

Schedule times and log timestamps are in this local time. Until the clock is synced, schedules wait and the log shows uptime (`+42s`) instead of a date.

### Dashboard login (optional)

```c
#define WEB_AUTH_USER       ""      // empty = no login
#define WEB_AUTH_PASS       ""
```

When set, every page and endpoint needs HTTP Basic Auth (`curl -u user:pass ...`).

### Internet monitor

```c
#define PING_HOST       "8.8.8.8"
#define PING_PROBE_SEC  15      // Seconds between checks (must divide 60)
#define PING_RESTART_AFTER_MIN 15   // Minutes without internet before restarting the router
#define PING_RELAY      0       // Which relay is the router
#define PING_GRACE_MIN  5       // Minutes to wait after restarting the router
```

Each check is a TCP connection to `PING_HOST` port 53 (works with public DNS servers like `8.8.8.8` or `1.1.1.1`), retried once before it counts as failed. A background task on the ESP32 does it, so the history is recorded whether or not the dashboard is open.

Checks are grouped per minute: average latency, how many checks failed, whether the ESP32 itself had lost WiFi, and the average WiFi signal (3 bytes per minute). The last 7 days (10,080 minutes, ~31 KB of RAM) are saved to the LittleFS data partition every 10 minutes and after each outage, so they survive reboots and OTA updates. The last 40 outages (2+ consecutive failed checks) are saved too. Checks made before the clock is synced (e.g. booting during an outage) are dated and added once NTP answers.

To answer "was it the internet?": pick a date and time in the dashboard, click the chart, an hour of the heatmap or an outage. The verdict says whether the internet was working at that minute, the exact span of any outage or intermittent drop around it, whether the ESP32 had lost WiFi (pointing at the WiFi/router rather than the line), or that there is no data (device off).

Manual checks from the dashboard are not stored and never trigger a restart. `PING_PROBE_SEC`, `PING_RESTART_AFTER_MIN`, `PING_GRACE_MIN` and the auth settings are optional: older `config.h` files without them still build (without `PING_RESTART_AFTER_MIN`, the old `PING_INTERVAL * PING_THRESHOLD` seconds are used). Needs the default partition table (it has the data partition); without it the history works but is lost on reboot.

## API Reference

The server listens on port 80. `JSON` endpoints are meant for integrations; actions use `POST` and answer `{"ok":true}` or `{"ok":false,"error":"..."}` with a 4xx/5xx status. The `GET` endpoints marked `HTML` open a human-readable confirmation page and run the action as soon as the URL is requested — for a browser, a phone shortcut or an NFC tag. A live, browsable version of this reference is also served at `/api-doc`.

### `POST /api/relays/restart?id=X` — JSON

Restarts a relay. Errors: `400` invalid id, `409` already restarting.

### `POST /api/wol/wake?id=X` — JSON

Sends a Wake-on-LAN magic packet. Errors: `400` invalid id, `503` WiFi not connected.

### `POST /api/ping/check` — JSON

Runs an internet check now; the result shows up in `ping` of `/api/status` a few seconds later.

### `GET /api/ping/history` — binary

Minute-by-minute history of the last 7 days. Optional `?from=<epoch minute>` returns only the minutes from there on.

20-byte little-endian header: `"PMH2"`, version (`2`), seconds between checks, 2 reserved bytes, `now` (epoch seconds, u32), first minute (epoch minute, u32), count (u32). Then 3 bytes per minute:

| Byte | Meaning |
|------|---------|
| `lat` | `0` = no successful check; `1-150` → `(v-1)*2` ms; `151-255` → `300+(v-151)*25` ms |
| `flags` | bits 0-2 checks done, bits 3-5 failed, bit 6 = ESP32 had no WiFi (all `0` = no data) |
| `rssi` | average WiFi signal, signed dBm (`0` = unknown) |

### `GET /api/ping/outages` — JSON

Last 40 outages, newest first: `[startEpoch, seconds, routerRestarted, noWifi]` (start `0` = before clock sync).

### `GET /api/schedule` — JSON

The configured restart schedules and the device's current time.

**Response:**
```json
{
  "time": "14:30",
  "synced": true,
  "schedules": [
    {
      "id": 0,
      "relay": 0,
      "relayName": "Router",
      "hour": 14,
      "min": 44,
      "enabled": true,
      "ranToday": false
    }
  ]
}
```

### `GET /api/log` — JSON

The last 50 system events, oldest first.

**Response:**
```json
[
  {"t": "29/07 14:30:12", "m": "Router: power OFF for 10s (web)"},
  {"t": "29/07 14:30:22", "m": "Router: power back ON"},
  {"t": "29/07 14:31:00", "m": "WoL magic packet sent to My Desktop PC"},
  {"t": "29/07 14:35:00", "m": "Ping 8.8.8.8 failed (1/3)"},
  {"t": "29/07 14:40:00", "m": "No internet after 3 checks, restarting Router"}
]
```

### `GET /` — HTML

The web dashboard (embedded in flash via `PROGMEM`).

### `GET /api-doc` — HTML

The API reference page described above.

### Any other route

Unknown `/api/...` paths return a JSON `404`; anything else falls back to the dashboard.

## Project Structure

```
PowerManager/
├── include/
│   ├── config.h.example  # Configuration template (no personal data, tracked in git)
│   ├── config.h          # Your real configuration (WiFi, relays, WoL, schedules, ping) — gitignored
│   ├── storage.h         # Config structs, optional-setting defaults, config loading
│   ├── util.h            # Time, uptime and JSON/HTML escaping helpers
│   ├── log.h             # Thread-safe event log (ring buffer)
│   ├── actions.h         # Queue that hands web requests over to loop()
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
