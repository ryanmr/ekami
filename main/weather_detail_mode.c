#include "weather_detail_mode.h"
#include "weather.h"
#include "ui.h"
#include "config.h"
#include "epaper_port.h"
#include "GUI_Paint.h"
#include "shtc3_bsp.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static const char *TAG = "wx_detail";

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

//
// Layout (800×480):
//
//  y=20
//         [weather icon 60r]       72F (Font48)
//         centered at (140, 90)    Feels like 68F (Font16)
//                                  Light Snow (Font24)
//  y=180  ── divider ──
//  y=200  Wind: 20 mph SE          Humidity: 37%
//  y=240  Pressure: 1009 hPa       Visibility: 0 km
//  y=280  Clouds: 100%             AQI: 3 (Moderate)
//  y=320  ── divider ──
//  y=345  Indoor: 68F  45% RH
//  y=380  PM2.5: 2.2
//  y=458  ● ○ ○ ○ ○ ○  page dots
//

// Monochrome weather icon — large version
void weather_draw_icon(int cx, int cy, int radius, const char *icon)
{
    if (!icon || strlen(icon) < 2) return;
    bool night = (strlen(icon) >= 3 && icon[2] == 'n');
    int r = radius;

    if (strncmp(icon, "01", 2) == 0) {
        if (night) {
            // Moon crescent
            Paint_DrawCircle(cx, cy, r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            Paint_DrawCircle(cx + r * 2/5, cy - r/4, r - 3, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        } else {
            // Sun with rays
            Paint_DrawCircle(cx, cy, r * 3/5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            for (int a = 0; a < 8; a++) {
                float angle = a * M_PI / 4.0f;
                int ri = r * 7/10;
                int ro = r;
                int x1 = cx + (int)(cosf(angle) * ri);
                int y1 = cy - (int)(sinf(angle) * ri);
                int x2 = cx + (int)(cosf(angle) * ro);
                int y2 = cy - (int)(sinf(angle) * ro);
                Paint_DrawLine(x1, y1, x2, y2, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
            }
        }
    } else if (strncmp(icon, "02", 2) == 0) {
        // Few clouds + sun/moon peeking
        if (!night) {
            Paint_DrawCircle(cx - r/3, cy - r/3, r * 2/5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            for (int a = 0; a < 8; a++) {
                float angle = a * M_PI / 4.0f;
                int x1 = cx - r/3 + (int)(cosf(angle) * (r/2));
                int y1 = cy - r/3 - (int)(sinf(angle) * (r/2));
                int x2 = cx - r/3 + (int)(cosf(angle) * (r * 3/5));
                int y2 = cy - r/3 - (int)(sinf(angle) * (r * 3/5));
                Paint_DrawLine(x1, y1, x2, y2, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            }
        } else {
            Paint_DrawCircle(cx - r/3, cy - r/3, r/3, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            Paint_DrawCircle(cx - r/3 + r/5, cy - r/3 - r/6, r/3 - 2, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        }
        // Cloud
        Paint_DrawCircle(cx + r/6, cy + r/8, r * 2/5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(cx - r/4, cy + r/4, r/4, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(cx + r/2, cy + r/4, r/4, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawRectangle(cx - r/4, cy + r/4, cx + r/2, cy + r/2,
            BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    } else if (strncmp(icon, "03", 2) == 0 || strncmp(icon, "04", 2) == 0) {
        // Overcast clouds
        Paint_DrawCircle(cx, cy - r/5, r * 2/5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(cx - r/2, cy + r/8, r/3, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(cx + r/2, cy + r/8, r/3, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawRectangle(cx - r/2, cy + r/8, cx + r/2, cy + r/3,
            BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    } else if (strncmp(icon, "09", 2) == 0 || strncmp(icon, "10", 2) == 0) {
        // Rain
        Paint_DrawCircle(cx, cy - r/4, r * 2/5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(cx - r/2, cy, r/4, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(cx + r/2, cy, r/4, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawRectangle(cx - r/2, cy, cx + r/2, cy + r/6, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        // Rain drops
        for (int d = -1; d <= 1; d++) {
            int dx = cx + d * r/3;
            Paint_DrawLine(dx, cy + r/3, dx - r/10, cy + r * 2/3,
                BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
        }
    } else if (strncmp(icon, "11", 2) == 0) {
        // Thunderstorm
        Paint_DrawCircle(cx, cy - r/4, r * 2/5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(cx - r/2, cy, r/4, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(cx + r/2, cy, r/4, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawRectangle(cx - r/2, cy, cx + r/2, cy + r/6, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        // Lightning bolt
        Paint_DrawLine(cx + r/8, cy + r/4, cx - r/6, cy + r/2, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
        Paint_DrawLine(cx - r/6, cy + r/2, cx + r/6, cy + r/2, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
        Paint_DrawLine(cx + r/6, cy + r/2, cx - r/8, cy + r * 4/5, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
    } else if (strncmp(icon, "13", 2) == 0) {
        // Snow
        Paint_DrawCircle(cx, cy - r/4, r * 2/5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(cx - r/2, cy, r/4, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(cx + r/2, cy, r/4, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawRectangle(cx - r/2, cy, cx + r/2, cy + r/6, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        // Snowflakes
        for (int d = -1; d <= 1; d++) {
            Paint_DrawCircle(cx + d * r/3, cy + r/2, r/10, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        }
    } else if (strncmp(icon, "50", 2) == 0) {
        // Fog
        for (int l = 0; l < 5; l++) {
            int lx1 = cx - r * 3/4;
            int lx2 = cx + r * 3/4;
            int ly = cy - r/2 + l * r/4;
            Paint_DrawLine(lx1, ly, lx2, ly, BLACK, DOT_PIXEL_2X2, LINE_STYLE_DOTTED);
        }
    }
}

void weather_detail_mode_render(uint8_t *image_buf, int current_mode, int total_modes)
{
    ESP_LOGI(TAG, "Rendering weather detail mode");

    Paint_NewImage(image_buf, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);
    Paint_SetScale(2);
    Paint_SelectImage(image_buf);
    Paint_Clear(WHITE);

    weather_data_t wx = weather_get_cached();

    if (!wx.valid) {
        int x = ui_center_x("No Weather Data", 24);
        Paint_DrawString_EN(x, 200, "No Weather Data", &Font24, WHITE, BLACK);
        ui_draw_page_dots(current_mode, total_modes);
        EPD_Display_Base(image_buf);
        return;
    }

    // === Top: large icon + temperature + info ===
    // Icon centered at (130, 90), radius 60
    weather_draw_icon(130, 90, 60, wx.icon);

    // Temperature (Font48: 40w × 83h) at y=30
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%dF", (int)roundf(wx.temp));
    Paint_DrawString_EN(230, 30, temp_str, &Font48, WHITE, BLACK);

    // Feels like (Font16: 16w × 28h) at y=115
    char feels_str[32];
    snprintf(feels_str, sizeof(feels_str), "Feels like %dF", (int)roundf(wx.feels_like));
    Paint_DrawString_EN(230, 115, feels_str, &Font16, WHITE, BLACK);

    // Description (Font24: 24w × 41h) at y=145
    Paint_DrawString_EN(230, 145, wx.description, &Font24, WHITE, BLACK);

    // === Divider ===
    Paint_DrawLine(40, 195, EPD_WIDTH - 40, 195, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // === Details — two columns, Font16 ===
    char buf[48];
    int c1 = 60;
    int c2 = 430;
    int row = 215;
    int rh = 35;

    snprintf(buf, sizeof(buf), "Wind: %d mph %s",
        (int)roundf(wx.wind_speed), weather_wind_dir(wx.wind_deg));
    Paint_DrawString_EN(c1, row, buf, &Font16, WHITE, BLACK);
    snprintf(buf, sizeof(buf), "Humidity: %d%%", wx.humidity);
    Paint_DrawString_EN(c2, row, buf, &Font16, WHITE, BLACK);

    row += rh;
    snprintf(buf, sizeof(buf), "Pressure: %d hPa", wx.pressure);
    Paint_DrawString_EN(c1, row, buf, &Font16, WHITE, BLACK);
    snprintf(buf, sizeof(buf), "Visibility: %d km", wx.visibility / 1000);
    Paint_DrawString_EN(c2, row, buf, &Font16, WHITE, BLACK);

    row += rh;
    snprintf(buf, sizeof(buf), "Clouds: %d%%", wx.clouds);
    Paint_DrawString_EN(c1, row, buf, &Font16, WHITE, BLACK);
    if (wx.aqi > 0) {
        snprintf(buf, sizeof(buf), "AQI: %d (%s)", wx.aqi, weather_aqi_label(wx.aqi));
        Paint_DrawString_EN(c2, row, buf, &Font16, WHITE, BLACK);
    }

    // === Divider ===
    row += rh + 5;
    Paint_DrawLine(40, row, EPD_WIDTH - 40, row, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    row += 15;

    // Indoor reading
    float temp_c = 0, humi_c = 0;
    if (SHTC3_GetTempAndHumiPolling(&temp_c, &humi_c) == NO_ERROR) {
        float temp_f = temp_c * 9.0f / 5.0f + 32.0f;
        snprintf(buf, sizeof(buf), "Indoor: %dF  %d%% RH", (int)roundf(temp_f), (int)roundf(humi_c));
        Paint_DrawString_EN(c1, row, buf, &Font16, WHITE, BLACK);
    }

    if (wx.pm2_5 > 0) {
        snprintf(buf, sizeof(buf), "PM2.5: %.1f ug/m3", wx.pm2_5);
        Paint_DrawString_EN(c2, row, buf, &Font16, WHITE, BLACK);
    }

    ui_draw_page_dots(current_mode, total_modes);

    EPD_Display_Base(image_buf);
    ESP_LOGI(TAG, "Weather detail rendered");
}
