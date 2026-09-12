# PowerManager

Sistema de gestion y reinicio de dispositivos basado en ESP32. Controla 4 reles para reiniciar equipos, envia Wake on LAN, ejecuta reinicios programados y monitorea la conexion a internet.

## Funcionalidades

- **4 Reles** — Control individual con tiempo de reinicio configurable por rele
- **Wake on LAN** — Enciende dispositivos remotos enviando paquetes magico
- **Reinicios Programados** — Hasta 10 horarios diarios para reiniciar reles
- **Ping Monitor** — Reinicio automatico del router si no hay internet
- **Dashboard Web** — Interfaz responsive con actualizacion automatica cada 3 segundos
- **LED de estado bicolor** — un color parpadea indicando que el sistema esta activo (ciclo), el otro se queda fijo mientras un rele esta reiniciando
- **WiFi** — Conexion con IP estatica, reconexion automatica

## Hardware

### Reles

| Rele | GPIO | Nombre por defecto | Tiempo OFF |
|------|------|-------------------|------------|
| 1 | 16 | Router / Discos / Switch | 10s |
| 2 | 17 | Servidor | 15s |
| 3 | 18 | NAS | 20s |
| 4 | 19 | Switch | 5s |

**Logica:** Los reles estan ON normalmente. Para reiniciar un dispositivo, el rele se apaga X segundos y luego vuelve a encender.

- `RELAY_ACTIVE = HIGH` — Nivel logico para activar el rele
- `RELAY_INACTIVE = LOW` — Nivel logico para desactivar el rele

### LED de estado bicolor

| Elemento | GPIO (config.h) |
|----------|------|
| Color "ciclo" (parpadea) | `STATUS_LED_CYCLE_PIN` (2) |
| Color "reinicio" (fijo mientras un rele reinicia) | `STATUS_LED_RESTART_PIN` (4) |

Conexion: cada color del LED bicolor con su propia resistencia (330 ohm) entre su GPIO y GND (o ánodo común a 3V3, segun el tipo de LED).

- El color de **ciclo** parpadea todo el tiempo mientras el firmware esta corriendo (equivalente al antiguo heartbeat).
- El color de **reinicio** se enciende fijo en cuanto **cualquier** rele entra en proceso de reinicio (apagado en espera de volver a encender) y se apaga cuando todos los reles vuelven a estar ON.

## Configuracion

Toda la configuracion se realiza por compilacion en `include/config.h`. No hay LittleFS ni configuracion runtime.

`include/config.h` esta en `.gitignore` porque contiene tu contrasena de WiFi y otros datos personales (IP, MAC de WoL, nombres de tus equipos) — nunca se sube al repositorio. Lo que se versiona es `include/config.h.example`, con la misma estructura pero valores de ejemplo. Para empezar:

```bash
cp include/config.h.example include/config.h
```

Y despues edita `include/config.h` con tus propios datos.

### LED de estado

```c
#define STATUS_LED_CYCLE_PIN      2      // Color 1: parpadeo de ciclo
#define STATUS_LED_CYCLE_INTERVAL 100    // ms entre cada parpadeo
#define STATUS_LED_RESTART_PIN    4      // Color 2: fijo durante reinicio de rele
```

### WiFi

```c
#define WIFI_SSID           "TuSSID"
#define WIFI_PASSWORD       "TuPassword"
#define WIFI_IP             "10.0.0.93"
#define WIFI_GATEWAY        "10.0.0.1"
#define WIFI_SUBNET         "255.255.0.0"
#define WIFI_DNS1           "8.8.8.8"
#define WIFI_DNS2           "1.1.1.1"
#define WIFI_CONNECT_TIMEOUT 15000
```

### Relés

```c
#define RELAY_COUNT         4
#define RELAY_ACTIVE        HIGH
#define RELAY_INACTIVE      LOW

#define RELAY_0_PIN         16
#define RELAY_0_NAME        "Router"
#define RELAY_0_RESTART_SEC 10
```

Repetir para `RELAY_1`, `RELAY_2`, `RELAY_3`.

### Wake on LAN

Maximo 5 dispositivos. Dejar nombre y MAC vacios para dispositivos no usados.

```c
#define WOL_0_NAME  "PC Despacho"
#define WOL_0_MAC   "F4:4D:30:18:58:95"
```

### Programacion de Reinicios

Maximo 10 entradas. Cada entrada significa "reiniciar este rele a esta hora".

```c
#define SCHED_0_ENABLED  true
#define SCHED_0_RELAY    0       // Indice del rele (0-3)
#define SCHED_0_HOUR     14      // Hora (0-23)
#define SCHED_0_MIN      44      // Minuto (0-59)
```

### Ping Monitor

```c
#define PING_HOST       "8.8.8.8"
#define PING_INTERVAL   300     // Segundos entre checks
#define PING_THRESHOLD  3       // Fallos seguidos para reiniciar
#define PING_RELAY      0       // Rele del router a reiniciar
```

## API REST

Todas las respuestas son JSON. El servidor escucha en el puerto 80. Todos los endpoints son GET.

### `GET /api/status`

Estado completo del sistema.

**Respuesta:**
```json
{
  "uptime": 3661,
  "datetime": "Lun 29 Jul 2026  14:30:00",
  "ip": "10.0.0.93",
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
      "name": "PC Despacho",
      "mac": "F4:4D:30:18:58:95"
    }
  ],
  "ping": {
    "host": "8.8.8.8",
    "interval": 300,
    "threshold": 3,
    "relayTrigger": 0,
    "failCount": 0,
    "lastResult": true
  }
}
```

### `GET /api/relays/restart?id=X`

Reinicia un rele (apaga X segundos, luego enciende).

**Parametros:**
- `id` (query) — Indice del rele (0-3)

**Respuesta exitosa:**
```json
{"ok": true}
```

**Error:**
```json
{"error": "Invalid relay ID"}
```

### `GET /api/wol/wake?id=X`

Envia paquete Wake on LAN a un dispositivo.

**Parametros:**
- `id` (query) — Indice del dispositivo (0-4)

**Respuesta exitosa:**
```json
{"ok": true}
```

**Error:**
```json
{"error": "Invalid ID"}
```

### `GET /api/schedule`

Lista de reinicios programados activos.

**Respuesta:**
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

### `GET /api/log`

Registro de eventos del sistema (maximo 50 entradas, cronologico).

**Respuesta:**
```json
[
  {"t": "29/07 14:30:12", "m": "Rele Router reiniciando (OFF 10s)"},
  {"t": "29/07 14:30:22", "m": "Rele Router ON (reinicio completo)"},
  {"t": "29/07 14:31:00", "m": "WoL enviado a PC Despacho"},
  {"t": "29/07 14:35:00", "m": "Programado: Servidor a las 05:00"},
  {"t": "29/07 14:40:00", "m": "Ping fallo 3 veces, reiniciando Router"}
]
```

### `GET /`

Dashboard web (HTML embebido en PROGMEM).

### `*` (cualquier otra ruta)

Sirve el dashboard web (SPA fallback).

## Compilar y Subir

### Requisitos

- [PlatformIO](https://platformio.org/)
- Board: `esp32doit-devkit-v1`

### Comandos

```bash
# Compilar
pio run

# Subir firmware
pio run --target upload

# Monitor serial
pio device monitor
```

## Estructura del Proyecto

```
PowerManager/
├── include/
│   ├── config.h.example # Plantilla de configuracion (sin datos personales, se versiona)
│   ├── config.h        # Tu configuracion real (WiFi, reles, WoL, horarios, ping) - gitignored
│   ├── storage.h       # Structs y carga de configuracion
│   ├── relay.h         # Control de reles (init, restart, update, JSON)
│   ├── wol.h           # Wake on LAN (init, send, JSON)
│   ├── scheduler.h     # Programacion de reinicios diarios
│   ├── ping.h          # Monitoreo de conexion a internet
│   └── webserver.h     # Servidor web + HTML embebido + API handlers
├── src/
│   └── main.cpp        # Setup, loop, WiFi, heartbeat LED
├── platformio.ini      # Configuracion de build
└── README.md
```

## Memoria

| Recurso | Uso | Total | Porcentaje |
|---------|-----|-------|------------|
| RAM | ~52 KB | 320 KB | ~16% |
| Flash | ~1.1 MB | 1.3 MB | ~84% |
