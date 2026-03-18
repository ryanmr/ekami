# ekami — ESP32-S3 ePaper Digital Badge

A digital badge and desk companion built on the **Waveshare ESP32-S3-ePaper-3.97-Kit** (SKU 33811), featuring a 3.97" e-paper display at 800×480 resolution with 6 interactive screens.

## Screens

Navigate between screens using the rotary encoder (left/right). Page indicator dots show your position.

### Badge
Name, title, and a dynamic QR code that encodes the current timestamp (`?f=ekami&dt=YYYY-MM-DD-HH-MM`). Shows indoor temperature/humidity and current weather at a glance. Corner accent design with letter-spaced domain.

### Clock
Large 12-hour time with AM/PM, full date, indoor sensor readings (SHTC3), and outdoor weather from OpenWeatherMap. Updates every minute via partial refresh with periodic full refresh to clear ghosting.

### Weather Detail
Large monochrome weather icon (sun, clouds, rain, snow, thunderstorm, fog — with day/night variants), temperature, feels-like, description, and a two-column details grid: wind, humidity, pressure, visibility, clouds, AQI, PM2.5, and indoor sensor.

### Sun Arc
Semicircular arc showing progress through the current day or night cycle. Moving dot indicates current position. Sunrise and sunset times at endpoints. Day/night length, time remaining countdown, and progress percentage. Ported from [candybar](https://github.com/ryanrampersad/candybar).

### Moon Phase
Moon circle with scanline-rendered illumination based on Julian day calculation. Phase name (New Moon through Waning Crescent), illumination percentage, cycle day, and predictions for next new/full moon. Ported from [candybar](https://github.com/ryanrampersad/candybar).

### Debug
System diagnostics: battery percentage, USB/battery status, WiFi SSID and RSSI, free heap and PSRAM, uptime, raw RTC values, weather cache freshness, and firmware build info.

## Features

- **OpenWeatherMap** integration with weather, AQI, and air pollution data
- **NVS weather cache** — persists across reboots, displays immediately on boot before WiFi connects
- **NTP time sync** — syncs system clock via WiFi, writes to PCF85063 RTC for offline timekeeping
- **Smart WiFi power** — always-on when USB-powered, 2-minute wake windows every 30 minutes on battery
- **Partial refresh** — 0.6s updates for clock mode without full-screen flash
- **Dynamic QR code** — encodes current timestamp for scan tracking
- **Rotary navigation** — left/right to change screens, press to advance, long-press for full refresh

## Hardware

| Component | Detail |
|---|---|
| MCU | ESP32-S3R8, dual-core LX7 @ 240MHz |
| Memory | 512KB SRAM, 8MB PSRAM, 16MB Flash |
| Display | 3.97" e-Paper, 800×480, 4 grayscale, partial refresh |
| Wireless | Wi-Fi 802.11 b/g/n, Bluetooth 5 LE |
| Sensors | SHTC3 (temp/humidity), QMI8658 (6-axis IMU) |
| RTC | PCF85063 with backup battery |
| Audio | ES8311 codec + NS4150B amp + mic |
| Storage | TF card slot (16GB card included) |
| Power | AXP2101 PMU, 3.7V LiPo battery, USB-C charging |
| Input | Rotary encoder (CW/CCW/press), BOOT button, PWR button |

## Setup

### Prerequisites

- ESP-IDF v5.5.0+ ([install guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/))
- `cmake`, `ninja` (via Homebrew on macOS: `brew install cmake ninja`)
- An [OpenWeatherMap](https://openweathermap.org/api) API key (free tier)

### Configuration

```bash
cp .env.example .env.local
# Edit .env.local with your WiFi credentials, OWM API key, city, and timezone
```

| Variable | Example | Description |
|---|---|---|
| `WIFI_SSID` | `MyNetwork` | WiFi network name |
| `WIFI_PASSWORD` | `secret` | WiFi password |
| `OWM_API_KEY` | `abc123...` | OpenWeatherMap API key |
| `OWM_CITY` | `Saint+Paul` | City for weather (URL-encoded) |
| `OWM_UNITS` | `imperial` | `imperial` (F) or `metric` (C) |
| `TIMEZONE_STR` | `CST6CDT,M3.2.0,M11.1.0` | POSIX timezone string |
| `BADGE_NAME` | `RYAN RAMPERSAD` | Name on badge screen |
| `BADGE_TAGLINE` | `Principal Software Engineer` | Title on badge screen |
| `BADGE_DETAIL` | `ryanrampersad.com` | Detail line (letter-spaced) |
| `BADGE_QR_URL` | `https://ryanrampersad.com` | Base URL for QR code |

### Build & Flash

```bash
./configure.sh                          # Generate main/config.h from .env.local
source ~/esp/esp-idf/export.sh          # Set up ESP-IDF environment
idf.py set-target esp32s3               # First time only
idf.py build                            # Build firmware
idf.py -p /dev/cu.usbmodem* flash      # Flash to board
```

If the board doesn't enter download mode automatically, hold **BOOT** while pressing **PWR**.

## Controls

| Input | Action |
|---|---|
| Rotary CW | Next screen |
| Rotary CCW | Previous screen |
| Rotary press | Next screen |
| Long press rotary | Force full refresh |
| Long press PWR | Power off |

## Project Structure

```
ekami/
├── main/
│   ├── main.c                  # App entry, mode manager, FreeRTOS tasks
│   ├── badge_mode.c/h          # Badge screen
│   ├── clock_mode.c/h          # Clock screen with partial refresh
│   ├── weather_detail_mode.c/h # Weather screen with icons
│   ├── sun_arc_mode.c/h        # Day/night arc screen
│   ├── moon_mode.c/h           # Moon phase screen
│   ├── debug_mode.c/h          # Debug stats screen
│   ├── weather.c/h             # OWM API client + NVS cache
│   ├── wifi_manager.c/h        # WiFi + NTP + smart power
│   ├── qr_generate.c/h         # QR code rendering
│   ├── ui.c/h                  # Shared UI helpers (page dots, centering)
│   ├── config.h.in             # Config template (envsubst)
│   └── qrcodegen.c/h           # QR library (nayuki, public domain)
├── components/                  # Waveshare BSP drivers
├── configure.sh                 # Generates config.h from .env.local
├── .env.example                 # Config template (safe to commit)
├── .env.local                   # Actual config (gitignored)
└── .dev/kb/                     # Hardware reference articles
```

## Related Projects

- **[candybar](https://github.com/ryanrampersad/candybar)** — ESP32-S3 desk status bar with LCD display. The sun arc, moon phase, weather icons, and OpenWeatherMap integration in ekami are ported from candybar's codebase.
- **[Waveshare ESP32-S3-ePaper-3.97 Demo](https://github.com/waveshareteam/ESP32-S3-ePaper-3.97)** — Official demo code and BSP drivers. The `components/` directory is derived from this repo.

## Display Specs

| Mode | Time |
|---|---|
| Full refresh | ~3.5s |
| Partial refresh | ~0.6s |

## Battery Life

In clock mode with WiFi wake windows every 30 minutes, a 1500mAh battery lasts **15+ days**.

## Knowledge Base

See `.dev/kb/` for detailed reference articles on hardware, setup, and development.
