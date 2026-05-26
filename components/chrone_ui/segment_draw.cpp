#include "segment_draw.hpp"

#include "clock_layout.h"

namespace {

constexpr uint8_t kSegmentPatterns[10] = {
    0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011,
    0b1011011, 0b1011111, 0b1110000, 0b1111111, 0b1111011,
};

constexpr int kScale = CHRONE_SEG_SCALE;
constexpr int kThick = CHRONE_SEG_THICK;
constexpr int kPad = CHRONE_SEG_PAD;

void canvas_fill_rect(lv_obj_t *canvas, int x, int y, int w, int h, lv_color_t color)
{
    if (w <= 0 || h <= 0) {
        return;
    }
    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            lv_canvas_set_px(canvas, x + col, y + row, color, LV_OPA_COVER);
        }
    }
}

void canvas_fill_circle(lv_obj_t *canvas, int cx, int cy, int r, lv_color_t color)
{
    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            if (dx * dx + dy * dy <= r * r) {
                lv_canvas_set_px(canvas, cx + dx, cy + dy, color, LV_OPA_COVER);
            }
        }
    }
}

void draw_bar_h(lv_obj_t *canvas, int x, int y, int w, lv_color_t color)
{
    const int r = kThick / 2;
    const int cy = y + r;
    if (w > 2 * r) {
        canvas_fill_rect(canvas, x + r, y, w - 2 * r, kThick, color);
    }
    canvas_fill_circle(canvas, x + r, cy, r, color);
    canvas_fill_circle(canvas, x + w - r - 1, cy, r, color);
}

void draw_bar_v(lv_obj_t *canvas, int x, int y, int h, lv_color_t color)
{
    const int r = kThick / 2;
    const int cx = x + r;
    if (h > 2 * r) {
        canvas_fill_rect(canvas, x, y + r, kThick, h - 2 * r, color);
    }
    canvas_fill_circle(canvas, cx, y + r, r, color);
    canvas_fill_circle(canvas, cx, y + h - r - 1, r, color);
}

void draw_bar_h_glow(lv_obj_t *canvas, int x, int y, int w, const chrone::ui::SegmentTheme &theme)
{
    draw_bar_h(canvas, x - 1, y - 1, w + 2, theme.glow);
    draw_bar_h(canvas, x, y, w, theme.on);
}

void draw_bar_v_glow(lv_obj_t *canvas, int x, int y, int h, const chrone::ui::SegmentTheme &theme)
{
    draw_bar_v(canvas, x - 1, y - 1, h + 2, theme.glow);
    draw_bar_v(canvas, x, y, h, theme.on);
}

void draw_segment_digit(lv_obj_t *canvas, int x, int y, int digit, const chrone::ui::SegmentTheme &theme)
{
    if (digit < 0 || digit > 9) {
        return;
    }

    const int dw = CHRONE_SEG_DIGIT_W;
    const int dh = CHRONE_SEG_DIGIT_H;
    canvas_fill_rect(canvas, x, y, dw, dh, theme.bg);

    const int inner_w = dw - 2 * kPad;
    const int inner_h = dh - 2 * kPad;
    const int half_h = (inner_h - kThick) / 2;
    const uint8_t pattern = kSegmentPatterns[digit];

    if (pattern & 0b1000000) {
        draw_bar_h_glow(canvas, x + kPad, y + kPad, inner_w, theme);
    }
    if (pattern & 0b0100000) {
        draw_bar_v_glow(canvas, x + dw - kPad - kThick, y + kPad + kThick, half_h, theme);
    }
    if (pattern & 0b0010000) {
        draw_bar_v_glow(canvas, x + dw - kPad - kThick, y + kPad + kThick + half_h + kThick, half_h, theme);
    }
    if (pattern & 0b0001000) {
        draw_bar_h_glow(canvas, x + kPad, y + dh - kPad - kThick, inner_w, theme);
    }
    if (pattern & 0b0000100) {
        draw_bar_v_glow(canvas, x + kPad, y + kPad + kThick + half_h + kThick, half_h, theme);
    }
    if (pattern & 0b0000010) {
        draw_bar_v_glow(canvas, x + kPad, y + kPad + kThick, half_h, theme);
    }
    if (pattern & 0b0000001) {
        draw_bar_h_glow(canvas, x + kPad, y + kPad + kThick + half_h, inner_w, theme);
    }
}

void draw_colon(lv_obj_t *canvas, int x, int y, const chrone::ui::SegmentTheme &theme, bool lit)
{
    const int r = (kThick + 1) / 2 + 1;
    const int cx = x + CHRONE_SEG_COLON_W / 2;
    const int y1 = y + CHRONE_SEG_DIGIT_H / 4;
    const int y2 = y + (CHRONE_SEG_DIGIT_H * 3) / 4;
    canvas_fill_rect(canvas, x, y, CHRONE_SEG_COLON_W, CHRONE_SEG_DIGIT_H, theme.bg);

    const lv_color_t c = lit ? theme.on : theme.colon_dim;
    const lv_color_t g = lit ? theme.glow : theme.bg;
    if (lit) {
        canvas_fill_circle(canvas, cx, y1, r + 1, g);
        canvas_fill_circle(canvas, cx, y2, r + 1, g);
    }
    canvas_fill_circle(canvas, cx, y1, r, c);
    canvas_fill_circle(canvas, cx, y2, r, c);
}

}  // namespace

namespace chrone::ui {

SegmentTheme segment_theme_default(void)
{
    return SegmentTheme{
        .bg = lv_color_hex(0x12151C),
        .on = lv_color_hex(0xFF6D3A),
        .glow = lv_color_hex(0x5C2208),
        .colon_dim = lv_color_hex(0x3A2818),
    };
}

int time_row_width(void)
{
    return 2 * CHRONE_SEG_PITCH + CHRONE_SEG_GAP_PAIR + CHRONE_SEG_COLON_W + CHRONE_SEG_GAP_COLON
           + 2 * CHRONE_SEG_PITCH + CHRONE_SEG_GAP_PAIR + CHRONE_SEG_COLON_W + CHRONE_SEG_GAP_COLON
           + 2 * CHRONE_SEG_PITCH;
}

void draw_hhmmss(lv_obj_t *canvas, int x0, int y, int hour, int minute, int second,
                 const SegmentTheme &theme, bool colon_lit)
{
    int x = x0;
    draw_segment_digit(canvas, x, y, hour / 10, theme);
    x += CHRONE_SEG_PITCH;
    draw_segment_digit(canvas, x, y, hour % 10, theme);
    x += CHRONE_SEG_PITCH + CHRONE_SEG_GAP_PAIR;
    draw_colon(canvas, x, y, theme, colon_lit);
    x += CHRONE_SEG_COLON_W + CHRONE_SEG_GAP_COLON;
    draw_segment_digit(canvas, x, y, minute / 10, theme);
    x += CHRONE_SEG_PITCH;
    draw_segment_digit(canvas, x, y, minute % 10, theme);
    x += CHRONE_SEG_PITCH + CHRONE_SEG_GAP_PAIR;
    draw_colon(canvas, x, y, theme, colon_lit);
    x += CHRONE_SEG_COLON_W + CHRONE_SEG_GAP_COLON;
    draw_segment_digit(canvas, x, y, second / 10, theme);
    x += CHRONE_SEG_PITCH;
    draw_segment_digit(canvas, x, y, second % 10, theme);
}

}  // namespace chrone::ui
