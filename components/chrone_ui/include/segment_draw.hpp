#pragma once

#include "lvgl.h"

namespace chrone::ui {

/** Retro LED palette (ref: ursweiss/7-Segment-LED-Clock, LVGL watch UIs) */
struct SegmentTheme {
    lv_color_t bg;
    lv_color_t on;
    lv_color_t glow;
    lv_color_t colon_dim;
};

SegmentTheme segment_theme_default(void);

int time_row_width(void);
void draw_hhmmss(lv_obj_t *canvas, int x0, int y, int hour, int minute, int second,
                 const SegmentTheme &theme, bool colon_lit);

}  // namespace chrone::ui
