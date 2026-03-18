#include "ui.h"
#include "GUI_Paint.h"
#include "epaper_port.h"
#include <string.h>

void ui_draw_page_dots(int current, int total)
{
    int dot_radius = 5;
    int dot_spacing = 20;
    int total_width = total * dot_spacing - (dot_spacing - dot_radius * 2);
    int start_x = (EPD_WIDTH - total_width) / 2;
    int y = EPD_HEIGHT - 22;

    for (int i = 0; i < total; i++) {
        int cx = start_x + i * dot_spacing;
        if (i == current) {
            Paint_DrawCircle(cx, y, dot_radius, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        } else {
            Paint_DrawCircle(cx, y, dot_radius, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
        }
    }
}

int ui_center_x(const char *text, int char_width)
{
    int len = strlen(text);
    int text_width = len * char_width;
    int x = (EPD_WIDTH - text_width) / 2;
    return (x < 0) ? 0 : x;
}
