#pragma once

#include "clock_layout.h"
#include "esp_err.h"
#include "lvgl.h"

struct tm;

namespace chrone::ui {

/** DS3231_Clock analog dial on LVGL canvas (landscape 320×240). */
class AnalogClockView {
public:
    AnalogClockView() = default;

    esp_err_t create(lv_obj_t *screen);
    void destroy();

    void paint_static_dial();
    void on_second_tick(const struct tm *tm);
    void force_hands_sync(const struct tm *tm);

    bool is_ready() const { return s_canvas != nullptr; }

private:
    void tip_from_angle(float angle_deg, int length, int *tx, int *ty) const;
    void erase_hand(int x1, int y1, int thick);
    void draw_hand(int x1, int y1, int thick, lv_color_t color);
    void draw_tick_index(int k);
    void redraw_all_ticks();
    void draw_center_cap(lv_color_t color);
    void create_hour_numerals(lv_obj_t *screen);
    void draw_all_hands(const struct tm *tm);

    lv_obj_t *s_canvas = nullptr;
    lv_obj_t *s_numeral_labels[12] = {};
    lv_color_t *s_buf = nullptr;

    int cx_ = 0;
    int cy_ = 0;
    int radius_ = CHRONE_ANALOG_RADIUS;

    lv_color_t col_face_{};
    lv_color_t col_ring_{};
    lv_color_t col_tick_minor_{};
    lv_color_t col_tick_major_{};
    lv_color_t col_hand_hour_{};
    lv_color_t col_hand_min_{};
    lv_color_t col_hand_sec_{};
    lv_color_t col_hub_{};

    bool hands_ok_ = false;
    int h_x1_ = 0, h_y1_ = 0, m_x1_ = 0, m_y1_ = 0, s_x1_ = 0, s_y1_ = 0;
    uint8_t last_drawn_sec_ = 0;

    int h_th_ = 5, m_th_ = 4, s_th_ = 2;
    int h_len_ = 34, m_len_ = 62, s_len_ = 75;
};

}  // namespace chrone::ui
