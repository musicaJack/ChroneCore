#include "chrone_weather_icon.h"
#include "weather_icon_map.h"

#include <string.h>

#define SRC_W 48
#define SRC_H 48
#define DST_W CHRONE_WEATHER_ICON_SZ
#define DST_H CHRONE_WEATHER_ICON_SZ

static int read_1bit_pixel(const uint8_t *bitmap, int x, int y)
{
    if (!bitmap || x < 0 || x >= SRC_W || y < 0 || y >= SRC_H) {
        return 1;
    }
    const int byte_ix = y * 6 + (x / 8);
    const int bit = 7 - (x % 8);
    return (bitmap[byte_ix] >> bit) & 1;
}

void chrone_weather_icon_render_rgb565(int code, bool is_day, uint16_t *out_rgb565,
                                       uint16_t fg_rgb565, uint16_t bg_rgb565)
{
    if (!out_rgb565) {
        return;
    }

    const uint8_t *bitmap = (code >= 0) ? weather_icon_get_bitmap(code, is_day) : NULL;
    for (int y = 0; y < DST_H; y++) {
        for (int x = 0; x < DST_W; x++) {
            const int sx = (x * SRC_W) / DST_W;
            const int sy = (y * SRC_H) / DST_H;
            int pixel = 1;
            if (bitmap) {
                pixel = read_1bit_pixel(bitmap, sx, sy);
            }
            out_rgb565[y * DST_W + x] = pixel ? bg_rgb565 : fg_rgb565;
        }
    }
}
