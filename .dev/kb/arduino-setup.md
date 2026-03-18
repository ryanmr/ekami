# Arduino IDE Development Setup

## Prerequisites

- Arduino IDE: https://www.arduino.cc/en/software/
- Use English-only file paths to avoid compatibility issues

## Board Manager Setup

1. Open **File → Preferences**
2. Add to "Additional Board Manager URLs":
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Open **Tools → Board → Board Manager**
4. Search "ESP32" and install the latest package
5. Select the appropriate ESP32-S3 board from **Tools → Board**

### Offline Package (Alternative)

Google Drive: https://drive.google.com/drive/folders/1YhHg8AA_02LFW2OqChk3hbFRV_GVt9Y0

## Required Libraries

| Library | Version | Install Method |
|---|---|---|
| SensorLib | v0.3.1 | Library Manager or offline |

All required libraries are also bundled in the demo repo under `Arduino/libraries/`.

## Demo Programs

| Demo | Description |
|---|---|
| 01_Audio_Test | ES8311 codec audio playback |
| 02_E-Paper_Example | Display init, images, shapes, text |
| 03_I2C_PCF85063 | RTC clock functionality |
| 04_I2C_SHTC3 | Temperature & humidity sensor |
| 05_SD_Test | Read BMP images from TF card |
| 06_QMI8658A | 6-axis IMU sensor data |

## Basic E-Paper Code Pattern

```cpp
EPD_3IN97_Init_Fast();
Paint_SelectImage(BlackImage);
Paint_Clear(WHITE);
Paint_DrawPoint(10, 80, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
EPD_3IN97_Display_Base(BlackImage);
```

## Troubleshooting

- **Can't upload**: Hold BOOT button while powering on to enter download mode
- **First compile is slow**: Expected — subsequent compiles are much faster
- **Compilation errors**: Verify Tools configuration matches your board
- **Serial port not found**: Check Device Manager (Windows) or `ls /dev/ttyUSB*` (Linux)
