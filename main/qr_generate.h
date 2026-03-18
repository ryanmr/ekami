#ifndef EKAMI_QR_GENERATE_H
#define EKAMI_QR_GENERATE_H

#include <stdint.h>
#include <stdbool.h>

// Generate a QR code and draw it onto the Paint image buffer.
// x, y: top-left position on the display
// pixel_size: size of each QR module in display pixels
// Returns true on success.
bool qr_draw(const char *text, int x, int y, int pixel_size);

#endif // EKAMI_QR_GENERATE_H
