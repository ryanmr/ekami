#include "clock_mode.h"
#include "config.h"
#include "weather.h"
#include "ui.h"
#include "epaper_port.h"
#include "GUI_Paint.h"
#include "pcf85063_bsp.h"
#include "shtc3_bsp.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

static const char *TAG = "clock";

static const char *day_names[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

static const char *month_names[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static int full_year(int rtc_year)
{
    return (rtc_year < 70) ? 2000 + rtc_year : 1900 + rtc_year;
}

void clock_mode_render(uint8_t *image_buf, int current_mode, int total_modes)
{
    ESP_LOGI(TAG, "Rendering clock mode");

    Paint_NewImage(image_buf, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);
    Paint_SetScale(2);
    Paint_SelectImage(image_buf);
    Paint_Clear(WHITE);

    Time_data rtc = PCF85063_GetTime();

    // Time — large centered
    char time_str[16];
    int hour_12 = rtc.hours % 12;
    if (hour_12 == 0) hour_12 = 12;
    const char *ampm = (rtc.hours < 12) ? "AM" : "PM";
    snprintf(time_str, sizeof(time_str), "%d:%02d %s", hour_12, rtc.minutes, ampm);

    // Font48: font48_Width_EN = 40
    int time_x = ui_center_x(time_str, 40);
    Paint_DrawString_EN(time_x, 100, time_str, &Font48, WHITE, BLACK);

    // Date
    char date_str[64];
    int dow = rtc.week;
    if (dow < 0 || dow > 6) dow = 0;
    int mon = rtc.months;
    if (mon < 1 || mon > 12) mon = 1;
    int year = full_year(rtc.years);
    snprintf(date_str, sizeof(date_str), "%s, %s %d, %d",
        day_names[dow], month_names[mon - 1], rtc.days, year);

    // Font24: font24_Width_EN = 24
    int date_x = ui_center_x(date_str, 24);
    Paint_DrawString_EN(date_x, 200, date_str, &Font24, WHITE, BLACK);

    // Divider
    Paint_DrawLine(50, 260, EPD_WIDTH - 50, 260, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Local sensor (SHTC3, C -> F)
    float temp = 0, humi = 0;
    if (SHTC3_GetTempAndHumiPolling(&temp, &humi) == NO_ERROR) {
        float temp_f = temp * 9.0f / 5.0f + 32.0f;
        char sensor_str[64];
        snprintf(sensor_str, sizeof(sensor_str), "%dF  %d%% RH", (int)roundf(temp_f), (int)roundf(humi));
        Paint_DrawString_EN(50, 290, "Indoor:", &Font16, WHITE, BLACK);
        Paint_DrawString_EN(50, 320, sensor_str, &Font24, WHITE, BLACK);
    }

    // Weather (from OWM cache) — temp on one line, description below
    weather_data_t wx = weather_get_cached();
    if (wx.valid) {
        char wx_temp[16];
        snprintf(wx_temp, sizeof(wx_temp), "%dF", (int)roundf(wx.temp));
        Paint_DrawString_EN(450, 290, "Outside:", &Font16, WHITE, BLACK);
        Paint_DrawString_EN(450, 320, wx_temp, &Font24, WHITE, BLACK);
        // Description in Font16 to avoid overflow (max 21 chars at 16px from x=450)
        char desc_trunc[22];
        strncpy(desc_trunc, wx.description, 21);
        desc_trunc[21] = '\0';
        Paint_DrawString_EN(450, 365, desc_trunc, &Font16, WHITE, BLACK);
    }

    // Footer with page dots
    Paint_DrawLine(50, EPD_HEIGHT - 50, EPD_WIDTH - 50, EPD_HEIGHT - 50,
        BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    ui_draw_page_dots(current_mode, total_modes);

    EPD_Display_Base(image_buf);
    ESP_LOGI(TAG, "Clock mode rendered");
}

void clock_mode_update_time(uint8_t *image_buf, int current_mode, int total_modes)
{
    Time_data rtc = PCF85063_GetTime();

    char time_str[16];
    int hour_12 = rtc.hours % 12;
    if (hour_12 == 0) hour_12 = 12;
    const char *ampm = (rtc.hours < 12) ? "AM" : "PM";
    snprintf(time_str, sizeof(time_str), "%d:%02d %s", hour_12, rtc.minutes, ampm);

    Paint_SelectImage(image_buf);

    // Clear and redraw time (wide margin to prevent ghosting)
    Paint_ClearWindows(0, 80, EPD_WIDTH, 195, WHITE);
    int time_x = ui_center_x(time_str, 40);
    Paint_DrawString_EN(time_x, 100, time_str, &Font48, WHITE, BLACK);

    // Clear and redraw sensor
    Paint_ClearWindows(50, 280, 430, 370, WHITE);
    float temp = 0, humi = 0;
    if (SHTC3_GetTempAndHumiPolling(&temp, &humi) == NO_ERROR) {
        float temp_f = temp * 9.0f / 5.0f + 32.0f;
        char sensor_str[64];
        snprintf(sensor_str, sizeof(sensor_str), "%dF  %d%% RH", (int)roundf(temp_f), (int)roundf(humi));
        Paint_DrawString_EN(50, 290, "Indoor:", &Font16, WHITE, BLACK);
        Paint_DrawString_EN(50, 320, sensor_str, &Font24, WHITE, BLACK);
    }

    // Clear and redraw weather
    weather_data_t wx = weather_get_cached();
    Paint_ClearWindows(450, 280, EPD_WIDTH - 10, 400, WHITE);
    if (wx.valid) {
        char wx_temp[16];
        snprintf(wx_temp, sizeof(wx_temp), "%dF", (int)roundf(wx.temp));
        Paint_DrawString_EN(450, 290, "Outside:", &Font16, WHITE, BLACK);
        Paint_DrawString_EN(450, 320, wx_temp, &Font24, WHITE, BLACK);
        char desc_trunc[22];
        strncpy(desc_trunc, wx.description, 21);
        desc_trunc[21] = '\0';
        Paint_DrawString_EN(450, 365, desc_trunc, &Font16, WHITE, BLACK);
    }

    EPD_Display_Partial(image_buf, 0, 0, EPD_WIDTH, EPD_HEIGHT);
    ESP_LOGI(TAG, "Clock updated (partial) %s", time_str);
}
