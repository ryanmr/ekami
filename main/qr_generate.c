#include "qr_generate.h"
#include "qrcodegen.h"
#include "GUI_Paint.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "qr";

bool qr_draw(const char *text, int x, int y, int pixel_size)
{
    uint8_t qr_buf[qrcodegen_BUFFER_LEN_FOR_VERSION(10)];
    uint8_t temp_buf[qrcodegen_BUFFER_LEN_FOR_VERSION(10)];

    bool ok = qrcodegen_encodeText(text,
        temp_buf, qr_buf,
        qrcodegen_Ecc_MEDIUM,
        qrcodegen_VERSION_MIN, 10,
        qrcodegen_Mask_AUTO, true);

    if (!ok) {
        ESP_LOGE(TAG, "QR encode failed for: %s", text);
        return false;
    }

    int size = qrcodegen_getSize(qr_buf);
    ESP_LOGI(TAG, "QR size: %d modules, pixel_size: %d, total: %dpx", size, pixel_size, size * pixel_size);

    // Draw white background with quiet zone
    int quiet = 2 * pixel_size;
    int total = size * pixel_size + 2 * quiet;
    Paint_DrawRectangle(x, y, x + total, y + total, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Draw QR modules
    for (int qy = 0; qy < size; qy++) {
        for (int qx = 0; qx < size; qx++) {
            if (qrcodegen_getModule(qr_buf, qx, qy)) {
                int px = x + quiet + qx * pixel_size;
                int py = y + quiet + qy * pixel_size;
                Paint_DrawRectangle(px, py, px + pixel_size - 1, py + pixel_size - 1,
                    BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            }
        }
    }

    return true;
}
