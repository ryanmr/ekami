#include "debug_mode.h"
#include "weather.h"
#include "wifi_manager.h"
#include "ui.h"
#include "config.h"
#include "epaper_port.h"
#include "GUI_Paint.h"
#include "pcf85063_bsp.h"
#include "axp_prot.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_heap_caps.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "debug";

void debug_mode_render(uint8_t *image_buf, int current_mode, int total_modes)
{
    ESP_LOGI(TAG, "Rendering debug screen");

    Paint_NewImage(image_buf, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);
    Paint_SetScale(2);
    Paint_SelectImage(image_buf);
    Paint_Clear(WHITE);

    int x = 40;
    int y = 30;
    int line_h = 35;
    char buf[80];

    Paint_DrawString_EN(x, y, "System Debug", &Font24, WHITE, BLACK);
    y += line_h + 10;

    Paint_DrawLine(x, y, EPD_WIDTH - x, y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    y += 15;

    // Battery
    int batt = get_battery_power();
    bool usb = get_usb_connected();
    snprintf(buf, sizeof(buf), "Battery: %d%%  USB: %s", batt, usb ? "Connected" : "Battery");
    Paint_DrawString_EN(x, y, buf, &Font16, WHITE, BLACK);
    y += line_h;

    // WiFi
    if (wifi_manager_is_connected()) {
        wifi_ap_record_t ap;
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            snprintf(buf, sizeof(buf), "WiFi: %s  RSSI: %d dBm", (char *)ap.ssid, ap.rssi);
        } else {
            snprintf(buf, sizeof(buf), "WiFi: Connected");
        }
    } else {
        snprintf(buf, sizeof(buf), "WiFi: Disconnected");
    }
    Paint_DrawString_EN(x, y, buf, &Font16, WHITE, BLACK);
    y += line_h;

    // Memory
    size_t free_heap = esp_get_free_heap_size();
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    snprintf(buf, sizeof(buf), "Heap: %u KB  PSRAM: %u KB",
        (unsigned)(free_heap / 1024), (unsigned)(free_psram / 1024));
    Paint_DrawString_EN(x, y, buf, &Font16, WHITE, BLACK);
    y += line_h;

    // Uptime
    int64_t uptime_us = esp_timer_get_time();
    int uptime_sec = (int)(uptime_us / 1000000);
    int hours = uptime_sec / 3600;
    int mins = (uptime_sec % 3600) / 60;
    int secs = uptime_sec % 60;
    snprintf(buf, sizeof(buf), "Uptime: %dh %dm %ds", hours, mins, secs);
    Paint_DrawString_EN(x, y, buf, &Font16, WHITE, BLACK);
    y += line_h;

    // RTC
    Time_data rtc = PCF85063_GetTime();
    int year = (rtc.years < 70) ? 2000 + rtc.years : 1900 + rtc.years;
    snprintf(buf, sizeof(buf), "RTC: %04d-%02d-%02d %02d:%02d:%02d (wday=%d)",
        year, rtc.months, rtc.days, rtc.hours, rtc.minutes, rtc.seconds, rtc.week);
    Paint_DrawString_EN(x, y, buf, &Font16, WHITE, BLACK);
    y += line_h;

    // Weather cache
    weather_data_t wx = weather_get_cached();
    if (wx.valid) {
        snprintf(buf, sizeof(buf), "Weather: %dF %s (fresh=%s)",
            (int)wx.temp, wx.description, weather_is_cache_fresh() ? "yes" : "no");
    } else {
        snprintf(buf, sizeof(buf), "Weather: No data cached");
    }
    Paint_DrawString_EN(x, y, buf, &Font16, WHITE, BLACK);
    y += line_h;

    // Firmware
    snprintf(buf, sizeof(buf), "Build: %s %s", __DATE__, __TIME__);
    Paint_DrawString_EN(x, y, buf, &Font16, WHITE, BLACK);
    y += line_h;

    snprintf(buf, sizeof(buf), "IDF: %s", esp_get_idf_version());
    Paint_DrawString_EN(x, y, buf, &Font16, WHITE, BLACK);

    ui_draw_page_dots(current_mode, total_modes);

    EPD_Display_Base(image_buf);
    ESP_LOGI(TAG, "Debug screen rendered");
}
