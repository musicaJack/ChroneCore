#include "alarm_time_picker.hpp"

#include "chrone_font_dseg56.h"
#include "chrone_haptic.h"
#include "chrone_time.h"
#include "clock_layout.h"

#include <cstdio>

namespace chrone::ui {

namespace {

constexpr int kColW = 100;
constexpr int kStepPx = 14;
/** 超过该位移视为拖动，不再触发单击 +1 */
constexpr int kTapSlopPx = 10;
/** 滚轮下方扩展触摸区（仅透明热区，不挡 Repeat 行） */
constexpr int kDragBelowPx = 10;

lv_color_t color_on(void) { return lv_color_hex(0xFFB020); }
lv_color_t color_muted(void) { return lv_color_hex(0x5A6478); }

int wrap_add(int v, int delta, int max_v)
{
    const int n = max_v + 1;
    int x = (v + delta) % n;
    if (x < 0) {
        x += n;
    }
    return x;
}

} // namespace

void AlarmTimePicker::style_seg_active(lv_obj_t *label)
{
    lv_obj_set_style_text_font(label, &chrone_font_dseg56, 0);
    lv_obj_set_style_text_color(label, color_on(), 0);
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
}

void AlarmTimePicker::style_seg_muted(lv_obj_t *label)
{
    lv_obj_set_style_text_font(label, &chrone_font_dseg56, 0);
    lv_obj_set_style_text_color(label, color_muted(), 0);
    lv_obj_set_style_text_opa(label, LV_OPA_50, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
}

void AlarmTimePicker::refresh_column(Column &col)
{
    if (!col.lbl_cur) {
        return;
    }

    char buf[8];
    std::snprintf(buf, sizeof(buf), "%02d", col.value);
    lv_label_set_text(col.lbl_cur, buf);

    if (col.col_id == s_active_col) {
        style_seg_active(col.lbl_cur);
    } else {
        style_seg_muted(col.lbl_cur);
    }
    lv_obj_invalidate(col.lbl_cur);
}

void AlarmTimePicker::set_active_column(int col_id)
{
    if (s_active_col == col_id) {
        return;
    }
    s_active_col = col_id;
    refresh_column(s_hour);
    refresh_column(s_minute);
}

void AlarmTimePicker::step_column(Column &col, int delta)
{
    const int before = col.value;
    col.value = wrap_add(col.value, delta, col.max_value);
    if (col.value == before) {
        return;
    }

    chrone_haptic_detent();
    refresh_column(col);
}

void AlarmTimePicker::process_drag_delta(Column &col, int dy)
{
    /* 仅下划调时；上划忽略（避免与顶栏/余量冲突） */
    if (dy <= 0) {
        return;
    }
    s_accum_dy += dy;
    while (s_accum_dy >= kStepPx) {
        step_column(col, -1);
        s_accum_dy -= kStepPx;
    }
}

void AlarmTimePicker::column_event_cb(lv_event_t *e)
{
    lv_obj_t *zone = lv_event_get_target_obj(e);
    auto *self = static_cast<AlarmTimePicker *>(lv_obj_get_user_data(zone));
    if (!self) {
        return;
    }

    const int col_id = (int)(intptr_t)lv_event_get_user_data(e);
    Column &col = (col_id == 0) ? self->s_hour : self->s_minute;

    const lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        self->set_active_column(col.col_id);
        self->s_dragging = true;
        self->s_drag_moved = false;
        self->s_accum_dy = 0;
        lv_indev_t *indev = lv_indev_active();
        if (indev) {
            lv_point_t pt;
            lv_indev_get_point(indev, &pt);
            self->s_last_pt_y = pt.y;
        }
        return;
    }

    if (code == LV_EVENT_PRESSING && self->s_dragging && self->s_active_col == col.col_id) {
        lv_indev_t *indev = lv_indev_active();
        if (!indev) {
            return;
        }
        lv_point_t pt;
        lv_indev_get_point(indev, &pt);
        const int dy = pt.y - self->s_last_pt_y;
        self->s_last_pt_y = pt.y;
        const int ady = dy < 0 ? -dy : dy;
        if (ady >= kTapSlopPx) {
            self->s_drag_moved = true;
        }
        self->process_drag_delta(col, dy);
        if (self->s_accum_dy > 0) {
            self->s_drag_moved = true;
        }
        return;
    }

    if (code == LV_EVENT_CLICKED) {
        self->set_active_column(col.col_id);
        if (!self->s_drag_moved) {
            self->step_column(col, 1);
        }
        self->s_drag_moved = false;
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        self->s_dragging = false;
        self->s_accum_dy = 0;
        if (code == LV_EVENT_PRESS_LOST) {
            self->s_drag_moved = false;
        }
    }
}

lv_obj_t *AlarmTimePicker::create(lv_obj_t *parent)
{
    destroy();
    if (!parent) {
        return nullptr;
    }

    const int root_h = CHRONE_ALARM_PICKER_H + kDragBelowPx;
    s_root = lv_obj_create(parent);
    lv_obj_set_size(s_root, CHRONE_LCD_W - 16, root_h);
    lv_obj_align(s_root, LV_ALIGN_TOP_MID, 0, CHRONE_ALARM_PICKER_Y);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_root, 0, 0);
    lv_obj_set_style_pad_all(s_root, 0, 0);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_flex_flow(s_root, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(s_root, 4, 0);

    auto setup_column = [this](Column &col, int max_v, int id) {
        col.max_value = max_v;
        col.col_id = id;
        col.zone = lv_obj_create(s_root);
        lv_obj_set_size(col.zone, kColW, root_h);
        lv_obj_set_style_bg_opa(col.zone, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(col.zone, 0, 0);
        lv_obj_remove_flag(col.zone, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(col.zone, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_user_data(col.zone, this);

        col.lbl_cur = lv_label_create(col.zone);
        lv_obj_align(col.lbl_cur, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_remove_flag(col.lbl_cur, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(col.lbl_cur, LV_OBJ_FLAG_EVENT_BUBBLE);
        col.lbl_prev = nullptr;
        col.lbl_next = nullptr;

        void *ud = (void *)(intptr_t)id;
        lv_obj_add_event_cb(col.zone, column_event_cb, LV_EVENT_PRESSED, ud);
        lv_obj_add_event_cb(col.zone, column_event_cb, LV_EVENT_PRESSING, ud);
        lv_obj_add_event_cb(col.zone, column_event_cb, LV_EVENT_CLICKED, ud);
        lv_obj_add_event_cb(col.zone, column_event_cb, LV_EVENT_RELEASED, ud);
        lv_obj_add_event_cb(col.zone, column_event_cb, LV_EVENT_PRESS_LOST, ud);
    };

    setup_column(s_hour, 23, 0);

    s_colon = lv_label_create(s_root);
    lv_label_set_text(s_colon, ":");
    style_seg_active(s_colon);

    setup_column(s_minute, 59, 1);

    s_active_col = 0;
    set_time(0, 0);
    refresh_column(s_hour);
    refresh_column(s_minute);
    return s_root;
}

void AlarmTimePicker::destroy()
{
    if (s_root) {
        lv_obj_delete(s_root);
    }
    s_root = nullptr;
    s_colon = nullptr;
    s_hour = {};
    s_minute = {};
}

void AlarmTimePicker::set_time(uint8_t hour, uint8_t minute)
{
    s_hour.value = hour % 24;
    s_minute.value = minute % 60;
    if (s_hour.lbl_cur) {
        refresh_column(s_hour);
        refresh_column(s_minute);
    }
}

void AlarmTimePicker::get_time(uint8_t *hour, uint8_t *minute) const
{
    if (hour) {
        *hour = (uint8_t)s_hour.value;
    }
    if (minute) {
        *minute = (uint8_t)s_minute.value;
    }
}

void AlarmTimePicker::set_default_now_plus_one()
{
    struct tm tm_local;
    if (chrone_time_get_local_tm(&tm_local) != ESP_OK) {
        set_time(0, 1);
        return;
    }
    int total = tm_local.tm_hour * 60 + tm_local.tm_min + 1;
    set_time((uint8_t)((total / 60) % 24), (uint8_t)(total % 60));
}

} // namespace chrone::ui
