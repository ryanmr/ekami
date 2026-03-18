# ESP-IDF Development Setup

## Version Requirement

**ESP-IDF v5.5.0 or higher** is recommended for the ESP32-S3-ePaper-3.97.

## Installation

### Offline Installer (Recommended)

1. Download from https://dl.espressif.com/dl/eim/
2. Get both files:
   - ESP-IDF Offline Package (.zst)
   - ESP-IDF Installer (.exe)
3. Run installer — it auto-detects the offline package
4. Select "Install from archive"
5. Use default path (avoid spaces/special characters)

### VS Code Integration

1. Install VS Code: https://code.visualstudio.com/
2. Install the ESP-IDF extension: `espressif.esp-idf-extension`
3. Extension v2.0+ auto-detects the ESP-IDF environment

## Common Initialization Pattern

```c
epaper_port_init();
EPD_Init();
i2c_master_init();
```

## Demo Code

The ESP-IDF demos are in the GitHub repo under `ESP-IDF/` with 8 integrated examples covering:
- E-Paper display operations
- I2C sensor communication (QMI8658, PCF85063, SHTC3)
- TF card mounting and file operations
- Audio codec initialization (ES8311)

## Flash Download Tool

For flashing pre-built firmware: https://dl.espressif.com/public/flash_download_tool.zip

## Resources

- Arduino-ESP32 docs: https://docs.espressif.com/projects/arduino-esp32/en/latest/index.html
- ESP32-S3 datasheet: https://documentation.espressif.com/esp32-s3_datasheet_en.pdf
- ESP32-S3 technical reference: https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf
