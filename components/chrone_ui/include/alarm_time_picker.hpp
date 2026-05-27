#pragma once

#include <cstdint>
#include "lvgl.h"

namespace chrone::ui {

/** DSEG7 双列 HH:MM：单击 +1，下划 -1（可绕回），上划无效 */
class AlarmTimePicker {
public:
    AlarmTimePicker() = default;

    lv_obj_t *create(lv_obj_t *parent);
    void destroy();

    void set_time(uint8_t hour, uint8_t minute);
    void get_time(uint8_t *hour, uint8_t *minute) const;

    /** 本地时钟 +1 分钟 */
    void set_default_now_plus_one();

    bool is_created() const { return s_root != nullptr; }
    lv_obj_t *root() const { return s_root; }

private:
    struct Column {
        lv_obj_t *zone = nullptr;
        lv_obj_t *lbl_cur = nullptr;
        lv_obj_t *lbl_prev = nullptr;
        lv_obj_t *lbl_next = nullptr;
        int value = 0;
        int max_value = 0;
        int col_id = 0;
    };

    lv_obj_t *s_root = nullptr;
    lv_obj_t *s_colon = nullptr;
    Column s_hour{};
    Column s_minute{};
    int s_active_col = 0;
    int s_accum_dy = 0;
    int s_last_pt_y = 0;
    bool s_dragging = false;
    bool s_drag_moved = false;

    void process_drag_delta(Column &col, int dy);

    static void style_seg_active(lv_obj_t *label);
    static void style_seg_muted(lv_obj_t *label);
    void refresh_column(Column &col);
    void set_active_column(int col_id);
    void step_column(Column &col, int delta);
    static void column_event_cb(lv_event_t *e);
};

} // namespace chrone::ui
