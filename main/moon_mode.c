#include "moon_mode.h"
#include "ui.h"
#include "epaper_port.h"
#include "GUI_Paint.h"
#include "pcf85063_bsp.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

static const char *TAG = "moon";

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static float moon_age(int year, int month, int day)
{
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;
    long jdn = day + (153L * m + 2) / 5 + 365L * y + y / 4 - y / 100 + y / 400 - 32045;
    float age = fmodf((float)(jdn - 2451550) + 0.5f, 29.53059f);
    return age < 0 ? age + 29.53059f : age;
}

static const char *moon_phase_name(float age)
{
    if (age < 1.85f)  return "New Moon";
    if (age < 5.53f)  return "Waxing Crescent";
    if (age < 9.22f)  return "First Quarter";
    if (age < 12.91f) return "Waxing Gibbous";
    if (age < 16.61f) return "Full Moon";
    if (age < 20.30f) return "Waning Gibbous";
    if (age < 23.99f) return "Last Quarter";
    if (age < 27.68f) return "Waning Crescent";
    return "New Moon";
}

void moon_mode_render(uint8_t *image_buf, int current_mode, int total_modes)
{
    ESP_LOGI(TAG, "Rendering moon phase");

    Paint_NewImage(image_buf, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);
    Paint_SetScale(2);
    Paint_SelectImage(image_buf);
    Paint_Clear(WHITE);

    // Use system time (set from NTP) for accurate date
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    int year = t.tm_year + 1900;
    int month = t.tm_mon + 1;
    int day = t.tm_mday;

    // Fallback to RTC if system time looks unset
    if (year < 2024) {
        Time_data rtc = PCF85063_GetTime();
        year = (rtc.years < 70) ? 2000 + rtc.years : 1900 + rtc.years;
        month = rtc.months;
        day = rtc.days;
    }

    float age = moon_age(year, month, day);
    float phase = age / 29.53059f;
    float illum = 0.5f * (1.0f - cosf(phase * 2.0f * M_PI));
    bool waxing = (age < 14.765f);

    ESP_LOGI(TAG, "Date: %04d-%02d-%02d, age=%.2f, illum=%.1f%%, waxing=%d",
        year, month, day, age, illum * 100, waxing);

    // Moon circle — large, centered
    int mcx = EPD_WIDTH / 2;
    int mcy = 170;
    int mr = 90;

    // Dark base
    Paint_DrawCircle(mcx, mcy, mr, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Illuminated portion via scanlines
    for (int row = -mr; row <= mr; row++) {
        float halfW = sqrtf((float)(mr * mr - row * row));
        float litW = halfW * illum * 2.0f;
        int x1, x2;
        if (waxing) {
            x1 = mcx + (int)(halfW - litW);
            x2 = mcx + (int)halfW;
        } else {
            x1 = mcx - (int)halfW;
            x2 = mcx - (int)(halfW - litW);
        }
        if (x2 > x1) {
            Paint_DrawLine(x1, mcy + row, x2, mcy + row, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        }
    }

    // Outline
    Paint_DrawCircle(mcx, mcy, mr, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

    // Phase name
    const char *name = moon_phase_name(age);
    int name_x = ui_center_x(name, 24);
    Paint_DrawString_EN(name_x, 285, name, &Font24, WHITE, BLACK);

    // Illumination
    char illum_str[24];
    snprintf(illum_str, sizeof(illum_str), "%d%% Illuminated", (int)roundf(illum * 100));
    int ill_x = ui_center_x(illum_str, 16);
    Paint_DrawString_EN(ill_x, 325, illum_str, &Font16, WHITE, BLACK);

    // Cycle info
    char cycle_str[48];
    snprintf(cycle_str, sizeof(cycle_str), "Day %.1f of 29.5  |  %04d-%02d-%02d",
        age, year, month, day);
    int cyc_x = ui_center_x(cycle_str, 16);
    Paint_DrawString_EN(cyc_x, 355, cycle_str, &Font16, WHITE, BLACK);

    // Next notable phases
    float days_to_new = 29.53059f - age;
    float days_to_full = (age < 14.765f) ? 14.765f - age : 14.765f + 29.53059f - age;
    char next_str[48];
    snprintf(next_str, sizeof(next_str), "New in %dd  |  Full in %dd",
        (int)roundf(days_to_new), (int)roundf(days_to_full));
    int next_x = ui_center_x(next_str, 16);
    Paint_DrawString_EN(next_x, 385, next_str, &Font16, WHITE, BLACK);

    ui_draw_page_dots(current_mode, total_modes);

    EPD_Display_Base(image_buf);
    ESP_LOGI(TAG, "Moon: %s, %.0f%% illum, age=%.1f", name, illum * 100, age);
}
