#include "chrone_alarm_icon.h"
#include "chrone_alarm_icon_bitmap.h"

#define BADGE_W 4
#define BADGE_H 6
#define BADGE_OX (CHRONE_ALARM_ICON_SZ - BADGE_W + CHRONE_ALARM_ICON_BADGE_GAP)

/** 4×6 数字点阵，行内高位为左 */
static const uint8_t k_badge_digit_4x6[10][BADGE_H] = {
    {0x0E, 0x0B, 0x0B, 0x0B, 0x0B, 0x0E}, /* 0 */
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x0E}, /* 1 */
    {0x0E, 0x02, 0x0E, 0x08, 0x08, 0x0E}, /* 2 */
    {0x0E, 0x02, 0x06, 0x02, 0x02, 0x0E}, /* 3 */
    {0x0B, 0x0B, 0x0E, 0x02, 0x02, 0x02}, /* 4 */
    {0x0E, 0x08, 0x0E, 0x02, 0x02, 0x0E}, /* 5 */
    {0x0E, 0x08, 0x0E, 0x0B, 0x0B, 0x0E}, /* 6 */
    {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02}, /* 7 */
    {0x0E, 0x0B, 0x0E, 0x0B, 0x0B, 0x0E}, /* 8 */
    {0x0E, 0x0B, 0x0E, 0x02, 0x02, 0x0E}, /* 9 */
};

static int read_1bit_pixel(const uint8_t *bitmap, int x, int y)
{
    if (!bitmap || x < 0 || x >= CHRONE_ALARM_ICON_SZ || y < 0 || y >= CHRONE_ALARM_ICON_SZ) {
        return 1;
    }
    const int byte_ix = y * 3 + (x / 8);
    const int bit = 7 - (x % 8);
    return (bitmap[byte_ix] >> bit) & 1;
}

static void draw_badge_digit(uint16_t *out_rgb565, uint8_t digit, uint16_t fg_rgb565)
{
    if (!out_rgb565 || digit > 9) {
        return;
    }
    const uint8_t *glyph = k_badge_digit_4x6[digit];
    for (int y = 0; y < BADGE_H; ++y) {
        const uint8_t row = glyph[y];
        for (int x = 0; x < BADGE_W; ++x) {
            if (!(row & (0x08 >> x))) {
                continue;
            }
            const int px = BADGE_OX + x;
            const int py = y;
            if (px < CHRONE_ALARM_ICON_CANVAS_W && py < CHRONE_ALARM_ICON_CANVAS_H) {
                out_rgb565[py * CHRONE_ALARM_ICON_CANVAS_W + px] = fg_rgb565;
            }
        }
    }
}

void chrone_alarm_icon_render_rgb565(uint16_t *out_rgb565, uint16_t fg_rgb565, uint16_t bg_rgb565,
                                    uint8_t badge_count)
{
    if (!out_rgb565) {
        return;
    }
    const uint8_t *bitmap = chrone_alarm_icon_bitmap_24;
    for (int y = 0; y < CHRONE_ALARM_ICON_CANVAS_H; y++) {
        for (int x = 0; x < CHRONE_ALARM_ICON_CANVAS_W; x++) {
            int pixel = 1;
            if (x < CHRONE_ALARM_ICON_SZ && y < CHRONE_ALARM_ICON_SZ) {
                pixel = read_1bit_pixel(bitmap, x, y);
            }
            out_rgb565[y * CHRONE_ALARM_ICON_CANVAS_W + x] = pixel ? bg_rgb565 : fg_rgb565;
        }
    }
    if (badge_count > 0) {
        draw_badge_digit(out_rgb565, badge_count, fg_rgb565);
    }
}
