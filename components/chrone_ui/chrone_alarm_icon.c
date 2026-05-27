#include "chrone_alarm_icon.h"
#include "chrone_alarm_icon_bitmap.h"

static int read_1bit_pixel(const uint8_t *bitmap, int x, int y)
{
    if (!bitmap || x < 0 || x >= CHRONE_ALARM_ICON_SZ || y < 0 || y >= CHRONE_ALARM_ICON_SZ) {
        return 1;
    }
    const int byte_ix = y * 3 + (x / 8);
    const int bit = 7 - (x % 8);
    return (bitmap[byte_ix] >> bit) & 1;
}

void chrone_alarm_icon_render_rgb565(uint16_t *out_rgb565, uint16_t fg_rgb565, uint16_t bg_rgb565)
{
    if (!out_rgb565) {
        return;
    }
    const uint8_t *bitmap = chrone_alarm_icon_bitmap_24;
    for (int y = 0; y < CHRONE_ALARM_ICON_SZ; y++) {
        for (int x = 0; x < CHRONE_ALARM_ICON_SZ; x++) {
            const int pixel = read_1bit_pixel(bitmap, x, y);
            out_rgb565[y * CHRONE_ALARM_ICON_SZ + x] = pixel ? bg_rgb565 : fg_rgb565;
        }
    }
}
