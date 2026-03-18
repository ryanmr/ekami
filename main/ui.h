#ifndef EKAMI_UI_H
#define EKAMI_UI_H

#include <stdint.h>

// Draw page indicator dots at bottom of screen
void ui_draw_page_dots(int current, int total);

// Calculate X position to center text on 800px display
int ui_center_x(const char *text, int char_width);

#endif // EKAMI_UI_H
