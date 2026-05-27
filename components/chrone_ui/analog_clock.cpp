/**
 * Analog clock — derived from DS3231_Clock (MIT) analog_clock.cpp / gfx_clock.cpp
 */

#include "analog_clock.hpp"

#include "clock_layout.h"

#include "esp_attr.h"
#include "esp_log.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

static const char *TAG = "analog_clk";

namespace chrone::ui {

namespace {

constexpr int kTickInnerInset = 10;
constexpr int kTickInnerInsetMinor = 6;
constexpr int kNumeralRadialGap = 2;
constexpr bool kMinuteHandSweep = false;

/** DS3231_Clock dial_palette RGB565 → LVGL (lv_color_hex 使用 0xRRGGBB，不能直接写 565 值) */
static lv_color_t color_from_rgb565(uint16_t c565)
{
    const uint8_t r = static_cast<uint8_t>(((c565 >> 11) & 0x1F) * 255 / 31);
    const uint8_t g = static_cast<uint8_t>(((c565 >> 5) & 0x3F) * 255 / 63);
    const uint8_t b = static_cast<uint8_t>((c565 & 0x1F) * 255 / 31);
    return lv_color_make(r, g, b);
}

void canvas_fill_rect(lv_obj_t *canvas, int x, int y, int w, int h, lv_color_t color)
{
    if (!canvas || w <= 0 || h <= 0) {
        return;
    }
    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            lv_canvas_set_px(canvas, x + col, y + row, color, LV_OPA_COVER);
        }
    }
}

void canvas_draw_line(lv_obj_t *canvas, int x0, int y0, int x1, int y1, int thickness, lv_color_t color)
{
    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    const int r = thickness / 2;

    for (;;) {
        canvas_fill_rect(canvas, x0 - r, y0 - r, thickness, thickness, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void canvas_draw_circle_outline(lv_obj_t *canvas, int cx, int cy, int radius, int thickness, lv_color_t color)
{
    if (radius <= 0 || thickness < 1) {
        return;
    }
    for (int rr = radius; rr > radius - thickness && rr > 0; --rr) {
        int x = rr;
        int y = 0;
        int err = 0;
        while (x >= y) {
            lv_canvas_set_px(canvas, cx + x, cy + y, color, LV_OPA_COVER);
            lv_canvas_set_px(canvas, cx + y, cy + x, color, LV_OPA_COVER);
            lv_canvas_set_px(canvas, cx - y, cy + x, color, LV_OPA_COVER);
            lv_canvas_set_px(canvas, cx - x, cy + y, color, LV_OPA_COVER);
            lv_canvas_set_px(canvas, cx - x, cy - y, color, LV_OPA_COVER);
            lv_canvas_set_px(canvas, cx - y, cy - x, color, LV_OPA_COVER);
            lv_canvas_set_px(canvas, cx + y, cy - x, color, LV_OPA_COVER);
            lv_canvas_set_px(canvas, cx + x, cy - y, color, LV_OPA_COVER);
            ++y;
            err += 1 + 2 * y;
            if (2 * (err - x) + 1 > 0) {
                --x;
                err += 1 - 2 * x;
            }
        }
    }
}

void canvas_fill_disc(lv_obj_t *canvas, int cx, int cy, int radius, lv_color_t color)
{
    for (int dy = -radius; dy <= radius; ++dy) {
        const int dx = static_cast<int>(std::sqrt(static_cast<float>(radius * radius - dy * dy)));
        canvas_fill_rect(canvas, cx - dx, cy + dy, 2 * dx + 1, 1, color);
    }
}

EXT_RAM_BSS_ATTR static lv_color_t s_analog_canvas_buf[CHRONE_ANALOG_CANVAS_SZ * CHRONE_ANALOG_CANVAS_SZ];

}  // namespace

esp_err_t AnalogClockView::create(lv_obj_t *screen)
{
    if (!screen) {
        return ESP_ERR_INVALID_ARG;
    }

    col_face_ = lv_color_hex(0x06080C);
    col_ring_ = color_from_rgb565(0x3186);
    col_tick_minor_ = color_from_rgb565(0x5ACB);
    col_tick_major_ = color_from_rgb565(0xDEFB);
    col_hand_hour_ = color_from_rgb565(0xFFDF);
    col_hand_min_ = color_from_rgb565(0xDEDB);
    col_hand_sec_ = color_from_rgb565(0xF800); /* RED in RGB565 */
    col_hub_ = color_from_rgb565(0xCE79);

    const int sz = CHRONE_ANALOG_CANVAS_SZ;
    cx_ = sz / 2;
    cy_ = sz / 2;
    s_buf = s_analog_canvas_buf;

    s_canvas = lv_canvas_create(screen);
    if (!s_canvas) {
        return ESP_ERR_NO_MEM;
    }

    lv_canvas_set_buffer(s_canvas, s_buf, sz, sz, LV_COLOR_FORMAT_RGB565);
    lv_obj_align(s_canvas, LV_ALIGN_TOP_MID, 0, CHRONE_HEADER_H - 4);
    lv_obj_remove_flag(s_canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_canvas, LV_OBJ_FLAG_CLICKABLE);

    create_hour_numerals(screen);
    paint_static_dial();
    hands_ok_ = false;

    ESP_LOGI(TAG, "analog dial %dx%d r=%d", sz, sz, radius_);
    return ESP_OK;
}

void AnalogClockView::detach()
{
    std::memset(s_numeral_labels, 0, sizeof(s_numeral_labels));
    s_canvas = nullptr;
    s_buf = nullptr;
    hands_ok_ = false;
}

void AnalogClockView::destroy()
{
    for (lv_obj_t *lb : s_numeral_labels) {
        if (lb) {
            lv_obj_delete(lb);
        }
    }
    detach();
}

void AnalogClockView::tip_from_angle(float angle_deg, int length, int *tx, int *ty) const
{
    const float rad = angle_deg * (3.14159265f / 180.f);
    *tx = cx_ + static_cast<int>(std::sin(rad) * static_cast<float>(length));
    *ty = cy_ - static_cast<int>(std::cos(rad) * static_cast<float>(length));
}

void AnalogClockView::erase_hand(int x1, int y1, int thick)
{
    canvas_draw_line(s_canvas, cx_, cy_, x1, y1, thick, col_face_);
}

void AnalogClockView::draw_hand(int x1, int y1, int thick, lv_color_t color)
{
    canvas_draw_line(s_canvas, cx_, cy_, x1, y1, thick, color);
}

void AnalogClockView::draw_tick_index(int k)
{
    k %= 60;
    if (k < 0) {
        k += 60;
    }
    const int r_out = radius_ - 2;
    const bool major = (k % 5) == 0;
    const int r_in = major ? (radius_ - kTickInnerInset) : (radius_ - kTickInnerInsetMinor);
    const float ang = static_cast<float>(k) * 6.f;
    int x0, y0, x1, y1;
    tip_from_angle(ang, r_in, &x0, &y0);
    tip_from_angle(ang, r_out, &x1, &y1);
    canvas_draw_line(s_canvas, x0, y0, x1, y1, major ? 3 : 2, major ? col_tick_major_ : col_tick_minor_);
}

void AnalogClockView::redraw_all_ticks()
{
    for (int k = 0; k < 60; ++k) {
        draw_tick_index(k);
    }
}

void AnalogClockView::draw_center_cap(lv_color_t color)
{
    canvas_fill_disc(s_canvas, cx_, cy_, 7, color);
}

void AnalogClockView::create_hour_numerals(lv_obj_t *screen)
{
    if (!screen || !s_canvas) {
        return;
    }

    /* 方案 A：与长刻度同角 θ=h×30°，数字中心落在固定半径 R_num，再按实测 bbox 居中 */
    const int r_tick_in = radius_ - kTickInnerInset;
    const float r_num = static_cast<float>(r_tick_in - kNumeralRadialGap - 8);

    lv_obj_update_layout(s_canvas);
    const int canvas_sx = lv_obj_get_x(s_canvas);
    const int canvas_sy = lv_obj_get_y(s_canvas);

    for (int h = 1; h <= 12; ++h) {
        const float ang_deg = static_cast<float>((h % 12) * 30);
        const float rad = ang_deg * (3.14159265f / 180.f);
        const float cx_num = static_cast<float>(cx_) + std::sin(rad) * r_num;
        const float cy_num = static_cast<float>(cy_) - std::cos(rad) * r_num;

        char buf[4];
        std::snprintf(buf, sizeof(buf), "%d", h);

        lv_obj_t *lb = lv_label_create(screen);
        lv_label_set_text(lb, buf);
        lv_obj_set_style_text_color(lb, lv_color_hex(0xE8ECF5), 0);
        lv_obj_set_style_text_font(lb, &lv_font_montserrat_14, 0);
        lv_obj_set_style_bg_opa(lb, LV_OPA_TRANSP, 0);
        lv_obj_set_style_pad_all(lb, 0, 0);
        lv_obj_update_layout(lb);

        const int32_t tw = lv_obj_get_width(lb);
        const int32_t th = lv_obj_get_height(lb);
        const int32_t left = canvas_sx + static_cast<int32_t>(cx_num + 0.5f) - tw / 2;
        const int32_t top = canvas_sy + static_cast<int32_t>(cy_num + 0.5f) - th / 2;
        lv_obj_set_pos(lb, left, top);
        s_numeral_labels[h - 1] = lb;
    }
}

void AnalogClockView::paint_static_dial()
{
    if (!s_canvas) {
        return;
    }
    lv_canvas_fill_bg(s_canvas, col_face_, LV_OPA_COVER);
    canvas_draw_circle_outline(s_canvas, cx_, cy_, radius_, 2, col_ring_);
    redraw_all_ticks();
    draw_center_cap(col_hub_);
    hands_ok_ = false;
    lv_obj_invalidate(s_canvas);
}

void AnalogClockView::draw_all_hands(const struct tm *tm)
{
    const int h12 = tm->tm_hour % 12;
    const float ang_h = (static_cast<float>(h12) + static_cast<float>(tm->tm_min) / 60.f +
                         static_cast<float>(tm->tm_sec) / 3600.f) * 30.f;
    const float ang_m = kMinuteHandSweep
                            ? (static_cast<float>(tm->tm_min) + static_cast<float>(tm->tm_sec) / 60.f) * 6.f
                            : static_cast<float>(tm->tm_min % 60) * 6.f;
    const float ang_s = static_cast<float>(tm->tm_sec) * 6.f;

    int hx, hy, mx, my, sx, sy;
    tip_from_angle(ang_h, h_len_, &hx, &hy);
    tip_from_angle(ang_m, m_len_, &mx, &my);
    tip_from_angle(ang_s, s_len_, &sx, &sy);

    draw_hand(hx, hy, h_th_, col_hand_hour_);
    draw_hand(mx, my, m_th_, col_hand_min_);
    draw_hand(sx, sy, s_th_, col_hand_sec_);

    h_x1_ = hx;
    h_y1_ = hy;
    m_x1_ = mx;
    m_y1_ = my;
    s_x1_ = sx;
    s_y1_ = sy;
    last_drawn_sec_ = static_cast<uint8_t>(tm->tm_sec % 60);
    hands_ok_ = true;
    draw_center_cap(col_hub_);
    lv_obj_invalidate(s_canvas);
}

void AnalogClockView::force_hands_sync(const struct tm *tm)
{
    if (hands_ok_) {
        erase_hand(h_x1_, h_y1_, h_th_);
        erase_hand(m_x1_, m_y1_, m_th_);
        erase_hand(s_x1_, s_y1_, s_th_);
        redraw_all_ticks();
    }
    draw_all_hands(tm);
}

void AnalogClockView::on_second_tick(const struct tm *tm)
{
    if (!s_canvas || !tm) {
        return;
    }

    const int h12 = tm->tm_hour % 12;
    const float ang_h = (static_cast<float>(h12) + static_cast<float>(tm->tm_min) / 60.f +
                         static_cast<float>(tm->tm_sec) / 3600.f) * 30.f;
    const float ang_m = kMinuteHandSweep
                            ? (static_cast<float>(tm->tm_min) + static_cast<float>(tm->tm_sec) / 60.f) * 6.f
                            : static_cast<float>(tm->tm_min % 60) * 6.f;
    const float ang_s = static_cast<float>(tm->tm_sec) * 6.f;

    int nhx, nhy, nmx, nmy, nsx, nsy;
    tip_from_angle(ang_h, h_len_, &nhx, &nhy);
    tip_from_angle(ang_m, m_len_, &nmx, &nmy);
    tip_from_angle(ang_s, s_len_, &nsx, &nsy);

    if (!hands_ok_) {
        draw_all_hands(tm);
        return;
    }

    const bool hour_moved = (nhx != h_x1_ || nhy != h_y1_);
    const bool minute_moved = (nmx != m_x1_ || nmy != m_y1_);

    erase_hand(s_x1_, s_y1_, s_th_);
    if (hour_moved) {
        erase_hand(h_x1_, h_y1_, h_th_);
    }
    if (minute_moved) {
        erase_hand(m_x1_, m_y1_, m_th_);
    }

    if (hour_moved || minute_moved) {
        redraw_all_ticks();
    } else {
        draw_tick_index(static_cast<int>(last_drawn_sec_));
    }

    draw_hand(nhx, nhy, h_th_, col_hand_hour_);
    draw_hand(nmx, nmy, m_th_, col_hand_min_);
    draw_hand(nsx, nsy, s_th_, col_hand_sec_);

    h_x1_ = nhx;
    h_y1_ = nhy;
    m_x1_ = nmx;
    m_y1_ = nmy;
    s_x1_ = nsx;
    s_y1_ = nsy;
    last_drawn_sec_ = static_cast<uint8_t>(tm->tm_sec % 60);

    draw_center_cap(col_hub_);
    lv_obj_invalidate(s_canvas);
}

}  // namespace chrone::ui
