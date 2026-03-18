#include "sun_arc_mode.h"
#include "weather.h"
#include "ui.h"
#include "epaper_port.h"
#include "GUI_Paint.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

static const char *TAG = "sun_arc";

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

//
// Layout (800×480):
//
//  y=20   "Daytime" or "Nighttime" title (Font24, centered)
//  y=70   ── divider ──
//  y=90
//         ╭──────────── sun arc (w=600, h=100) ────────────╮
//  y=280  ╰── baseline ───────────────────────────────────╯
//  y=295  sunrise_time              [label]           sunset_time
//  y=330  ── divider ──
//  y=350  Sunrise: 7:12 AM    Sunset: 7:28 PM    Day: 12h 16m
//  y=385  X hours remaining of daylight / nighttime
//  y=458  ● ○ ○ ○ ○ ○  page dots
//

static void draw_sun_arc(int arc_x, int arc_y, int arc_w, int arc_h,
                         weather_data_t *wx, bool *out_is_day, float *out_progress)
{
    time_t now = time(NULL);
    time_t rise = (time_t)wx->sunrise;
    time_t set = (time_t)wx->sunset;

    if (set <= rise) {
        *out_is_day = true;
        *out_progress = 0.5f;
        return;
    }

    bool is_day = (now >= rise && now < set);
    float progress;

    if (is_day) {
        float duration = (float)(set - rise);
        progress = (duration > 0) ? (float)(now - rise) / duration : 0.5f;
    } else {
        time_t night_start, night_end;
        if (now >= set) {
            night_start = set;
            night_end = rise + 86400;
        } else {
            night_start = set - 86400;
            night_end = rise;
        }
        float duration = (float)(night_end - night_start);
        progress = (duration > 0) ? (float)(now - night_start) / duration : 0.5f;
    }
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    *out_is_day = is_day;
    *out_progress = progress;

    int segments = 40;
    int night_base_y = arc_y - arc_h + 3;

    // Draw arc
    for (int i = 0; i < segments; i++) {
        float a1 = M_PI * (float)i / segments;
        float a2 = M_PI * (float)(i + 1) / segments;
        int x1 = arc_x + (int)(a1 / M_PI * arc_w);
        int x2 = arc_x + (int)(a2 / M_PI * arc_w);
        int y1, y2;
        if (is_day) {
            y1 = arc_y - (int)(sinf(a1) * arc_h);
            y2 = arc_y - (int)(sinf(a2) * arc_h);
        } else {
            y1 = night_base_y + (int)(sinf(a1) * arc_h);
            y2 = night_base_y + (int)(sinf(a2) * arc_h);
        }
        Paint_DrawLine(x1, y1, x2, y2, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    }

    // Baseline
    Paint_DrawLine(arc_x, arc_y, arc_x + arc_w, arc_y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

    // Moving dot
    float dot_angle = M_PI * progress;
    int dot_x = arc_x + (int)(dot_angle / M_PI * arc_w);
    int dot_y;
    if (is_day) {
        dot_y = arc_y - (int)(sinf(dot_angle) * arc_h);
    } else {
        dot_y = night_base_y + (int)(sinf(dot_angle) * arc_h);
    }
    Paint_DrawCircle(dot_x, dot_y, 7, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    // White center for visibility
    Paint_DrawCircle(dot_x, dot_y, 3, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

void sun_arc_mode_render(uint8_t *image_buf, int current_mode, int total_modes)
{
    ESP_LOGI(TAG, "Rendering sun arc mode");

    Paint_NewImage(image_buf, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);
    Paint_SetScale(2);
    Paint_SelectImage(image_buf);
    Paint_Clear(WHITE);

    weather_data_t wx = weather_get_cached();

    if (!wx.valid || wx.sunrise == 0 || wx.sunset == 0) {
        int x = ui_center_x("No Sun Data", 24);
        Paint_DrawString_EN(x, 200, "No Sun Data", &Font24, WHITE, BLACK);
        ui_draw_page_dots(current_mode, total_modes);
        EPD_Display_Base(image_buf);
        return;
    }

    // Draw the arc — large and centered
    bool is_day = true;
    float progress = 0;
    int arc_x = 100;
    int arc_w = 600;
    int arc_y = 280;
    int arc_h = 100;
    draw_sun_arc(arc_x, arc_y, arc_w, arc_h, &wx, &is_day, &progress);

    // Title
    const char *title = is_day ? "Daytime" : "Nighttime";
    int title_x = ui_center_x(title, 24);
    Paint_DrawString_EN(title_x, 25, title, &Font24, WHITE, BLACK);

    // Divider above arc
    Paint_DrawLine(60, 65, EPD_WIDTH - 60, 65, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Sunrise/sunset time labels below arc
    time_t rise = (time_t)wx.sunrise;
    time_t set = (time_t)wx.sunset;
    struct tm rise_tm, set_tm;
    localtime_r(&rise, &rise_tm);
    localtime_r(&set, &set_tm);

    int rh = rise_tm.tm_hour % 12; if (rh == 0) rh = 12;
    int sh = set_tm.tm_hour % 12; if (sh == 0) sh = 12;

    char rbuf[16], sbuf[16];
    snprintf(rbuf, sizeof(rbuf), "%d:%02d %s", rh, rise_tm.tm_min, rise_tm.tm_hour < 12 ? "AM" : "PM");
    snprintf(sbuf, sizeof(sbuf), "%d:%02d %s", sh, set_tm.tm_min, set_tm.tm_hour < 12 ? "AM" : "PM");

    // Font16: 16w × 28h
    Paint_DrawString_EN(arc_x, arc_y + 10, rbuf, &Font16, WHITE, BLACK);
    int sbuf_w = strlen(sbuf) * 16;
    Paint_DrawString_EN(arc_x + arc_w - sbuf_w, arc_y + 10, sbuf, &Font16, WHITE, BLACK);

    // Divider below labels
    Paint_DrawLine(60, 330, EPD_WIDTH - 60, 330, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Info row
    char buf[64];
    int info_y = 345;

    // Day length
    int day_mins = (int)((wx.sunset - wx.sunrise) / 60);
    snprintf(buf, sizeof(buf), "Day: %dh %dm", day_mins / 60, day_mins % 60);
    Paint_DrawString_EN(60, info_y, buf, &Font16, WHITE, BLACK);

    // Night length
    int night_mins = 24 * 60 - day_mins;
    snprintf(buf, sizeof(buf), "Night: %dh %dm", night_mins / 60, night_mins % 60);
    Paint_DrawString_EN(320, info_y, buf, &Font16, WHITE, BLACK);

    // Time remaining
    time_t now = time(NULL);
    int remaining_mins;
    if (is_day) {
        remaining_mins = (int)((set - now) / 60);
        if (remaining_mins < 0) remaining_mins = 0;
        int rem_h = remaining_mins / 60;
        int rem_m = remaining_mins % 60;
        snprintf(buf, sizeof(buf), "%dh %dm of daylight left", rem_h, rem_m);
    } else {
        time_t next_rise = (now >= set) ? rise + 86400 : rise;
        remaining_mins = (int)((next_rise - now) / 60);
        if (remaining_mins < 0) remaining_mins = 0;
        int rem_h = remaining_mins / 60;
        int rem_m = remaining_mins % 60;
        snprintf(buf, sizeof(buf), "%dh %dm until sunrise", rem_h, rem_m);
    }
    int rem_x = ui_center_x(buf, 16);
    Paint_DrawString_EN(rem_x, info_y + 35, buf, &Font16, WHITE, BLACK);

    // Progress percentage
    char pct_str[16];
    snprintf(pct_str, sizeof(pct_str), "%d%%", (int)roundf(progress * 100));
    int pct_x = ui_center_x(pct_str, 24);
    Paint_DrawString_EN(pct_x, info_y + 65, pct_str, &Font24, WHITE, BLACK);

    ui_draw_page_dots(current_mode, total_modes);

    EPD_Display_Base(image_buf);
    ESP_LOGI(TAG, "Sun arc rendered (%s, %d%% through)", is_day ? "day" : "night", (int)(progress * 100));
}
