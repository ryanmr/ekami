#include "badge_mode.h"
#include "config.h"
#include "qr_generate.h"
#include "weather.h"
#include "ui.h"
#include "epaper_port.h"
#include "GUI_Paint.h"
#include "shtc3_bsp.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>
#include <time.h>

static const char *TAG = "badge";

// Draw text with extra letter spacing (pixels between each char)
static void draw_spaced_text(int x, int y, const char *text, sFONT *font, int spacing)
{
    while (*text) {
        Paint_DrawChar(x, y, *text, font, WHITE, BLACK);
        x += font->Width + spacing;
        text++;
    }
}

// Draw a decorative geometric border pattern
static void draw_corner_accents(int x1, int y1, int x2, int y2, int size)
{
    // Top-left
    Paint_DrawLine(x1, y1, x1 + size, y1, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(x1, y1, x1, y1 + size, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    // Top-right
    Paint_DrawLine(x2 - size, y1, x2, y1, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(x2, y1, x2, y1 + size, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    // Bottom-left
    Paint_DrawLine(x1, y2, x1 + size, y2, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(x1, y2 - size, x1, y2, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    // Bottom-right
    Paint_DrawLine(x2 - size, y2, x2, y2, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(x2, y2 - size, x2, y2, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
}

void badge_mode_render(uint8_t *image_buf, int current_mode, int total_modes)
{
    ESP_LOGI(TAG, "Rendering badge mode");

    // Layout map (800×480):
    //
    //  y=0
    //  y=15   ┌─ corner accents ─┐
    //  y=25   RYAN RAMPERSAD (Font48: 40w×83h) → y=25 to y=108
    //  y=115  ── thin divider ──
    //  y=125  Principal Software Engineer (Font24: 24w×41h) → to y=166
    //  y=175  r y a n r a m p e r s a d . c o m  (Font16 spaced)
    //  y=220  ── divider ──
    //  y=235  [indoor temp/humidity]  [weather summary]      ┌──────┐
    //  y=280                                                 │  QR  │
    //  y=350                                                 └──────┘
    //  y=430  ── footer ──
    //  y=458  ● ○ ○ ○ ○  page dots
    //  y=480

    Paint_NewImage(image_buf, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);
    Paint_SetScale(2);
    Paint_SelectImage(image_buf);
    Paint_Clear(WHITE);

    // Corner accents instead of full border
    draw_corner_accents(15, 15, EPD_WIDTH - 15, EPD_HEIGHT - 15, 40);

    // Name (Font48: 40w × 83h)
    Paint_DrawString_EN(40, 25, BADGE_NAME, &Font48, WHITE, BLACK);

    // Thin divider
    Paint_DrawLine(40, 115, EPD_WIDTH - 40, 115, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Title (Font24: 24w × 41h)
    Paint_DrawString_EN(40, 125, BADGE_TAGLINE, &Font24, WHITE, BLACK);

    // Domain with letter spacing (Font16: 16w × 28h, +4px extra spacing)
    draw_spaced_text(40, 180, BADGE_DETAIL, &Font16, 4);

    // Divider before info section
    Paint_DrawLine(40, 220, EPD_WIDTH - 40, 220, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // QR code — dynamic URL with timestamp
    // ~54 chars → QR version ~5 (37 modules), pixel_size=4 → ~164px
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    char qr_url[128];
    snprintf(qr_url, sizeof(qr_url), "%s?f=ekami&dt=%04d-%02d-%02d-%02d-%02d",
        BADGE_QR_URL,
        t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
        t.tm_hour, t.tm_min);
    ESP_LOGI(TAG, "QR URL: %s", qr_url);

    int qr_x = EPD_WIDTH - 230;
    int qr_y = 230;
    qr_draw(qr_url, qr_x, qr_y, 4);

    // Sensor reading — left side
    float temp = 0, humi = 0;
    if (SHTC3_GetTempAndHumiPolling(&temp, &humi) == NO_ERROR) {
        float temp_f = temp * 9.0f / 5.0f + 32.0f;
        char sensor_str[32];
        snprintf(sensor_str, sizeof(sensor_str), "%dF indoor  %d%% RH",
            (int)roundf(temp_f), (int)roundf(humi));
        Paint_DrawString_EN(40, 240, sensor_str, &Font16, WHITE, BLACK);
    }

    // Weather summary — left side, below sensor
    weather_data_t wx = weather_get_cached();
    if (wx.valid) {
        char wx_str[48];
        snprintf(wx_str, sizeof(wx_str), "%dF outside  %s",
            (int)roundf(wx.temp), wx.description);
        Paint_DrawString_EN(40, 275, wx_str, &Font16, WHITE, BLACK);
    }

    // Decorative dots (a row of small circles as a visual accent)
    for (int i = 0; i < 12; i++) {
        int dx = 40 + i * 18;
        Paint_DrawCircle(dx, 330, 2, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }

    // Small "ekami" branding
    Paint_DrawString_EN(40, 400, "ekami", &Font16, WHITE, BLACK);

    // Page dots
    ui_draw_page_dots(current_mode, total_modes);

    EPD_Display_Base(image_buf);
    ESP_LOGI(TAG, "Badge mode rendered");
}
