#ifndef EKAMI_CLOCK_MODE_H
#define EKAMI_CLOCK_MODE_H

#include <stdint.h>

void clock_mode_render(uint8_t *image_buf, int current_mode, int total_modes);
void clock_mode_update_time(uint8_t *image_buf, int current_mode, int total_modes);

#endif // EKAMI_CLOCK_MODE_H
