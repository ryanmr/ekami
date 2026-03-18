# Hardware Specifications

## ESP32-S3R8 MCU

- Dual-core Xtensa 32-bit LX7 processor, up to 240MHz
- 512KB SRAM, 384KB ROM
- 8MB PSRAM (stacked on chip)
- 16MB external Flash
- 2.4GHz Wi-Fi 802.11 b/g/n
- Bluetooth 5 LE with onboard antenna

## 3.97" e-Paper Display

- Resolution: 800 × 480 pixels
- 4 grayscale levels
- Full refresh: ~3.5 seconds
- Partial refresh: ~0.6 seconds
- High contrast, wide viewing angle
- Ultra-low power — retains image without power
- Ambient light readable (no backlight needed)

## Onboard Sensors

### SHTC3 — Temperature & Humidity
- Datasheet: https://files.waveshare.com/wiki/common/SHTC3_Datasheet.pdf
- I2C interface

### QMI8658 — 6-Axis IMU
- 3-axis accelerometer + 3-axis gyroscope
- Useful for orientation detection (portrait/landscape auto-rotate)

### PCF85063 — Real-Time Clock
- Datasheet: https://files.waveshare.com/wiki/common/Pcf85063atl1118-NdPQpTGE-loeW7GbZ7.pdf
- Backup battery support via MX1.25 connector
- Keeps time when main power is off

## Audio System

- ES8311 codec chip (datasheet: https://files.waveshare.com/wiki/common/ES8311.DS.pdf)
- NS4150B amplifier
- Built-in microphone
- Kit includes 8Ω 1W speaker

## Power Management

- TG28 power management chip
- 3.7V MX1.25 lithium battery connector
- Integrated charging via USB-C
- Multiple configurable output voltages
- Clock mode battery life: 15+ days on 1500mAh

## Connectors & Interfaces

- USB Type-C — programming, serial output, charging
- TF card slot — FAT32, microSD (16GB card included in kit)
- MX1.25 connectors for battery and RTC backup battery

## Input Controls

- **Rotary button** — 3-direction (rotate CW, rotate CCW, press)
- **BOOT button** — hold during power-on to enter download mode
- **PWR button** — power management when battery-powered

## Board Schematic

https://files.waveshare.com/wiki/ESP32-S3-ePaper-3.97/ESP32-S3_e-Paper-3.97-schematic.pdf
