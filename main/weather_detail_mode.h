#ifndef EKAMI_WEATHER_DETAIL_MODE_H
#define EKAMI_WEATHER_DETAIL_MODE_H

#include <stdint.h>

void weather_detail_mode_render(uint8_t *image_buf, int current_mode, int total_modes);
void weather_draw_icon(int cx, int cy, int radius, const char *icon);

#endif
