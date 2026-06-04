#include "chrone_ui.h"
#include "chrone_ui_layout.h"

#include "alarm_time_picker.hpp"
#include "chrone_alarm.h"
#include "chrone_display_idle.h"
#include "chrone_haptic.h"
#include "chrone_input.h"
#include "clock_layout.h"

#include "esp_log.h"
#include <cstdio>
#include <cstring>

static const char *TAG = "chrone_ui_alarm";

static lv_obj_t *s_screen;
static lv_obj_t *s_list_panel;
static lv_obj_t *s_edit_panel;
static int s_edit_index = -1;
static bool s_alarm_ui_active;

static lv_obj_t *s_sw_enabled;
static lv_obj_t *s_lbl_repeat;
static lv_obj_t *s_lbl_rings_at;
static lv_obj_t *s_header_bar;
static uint8_t s_edit_repeat;
static chrone::ui::AlarmTimePicker s_time_picker;

static lv_obj_t *s_error_toast;
static lv_timer_t *s_error_toast_timer;

#define ALARM_ERROR_TOAST_MS 5000

static const char *repeat_str(uint8_t r)
{
    switch (r) {
    case CHRONE_ALARM_REPEAT_WORKDAY:
        return "Workday";
    case CHRONE_ALARM_REPEAT_ONCE:
        return "Once";
    case CHRONE_ALARM_REPEAT_DAILY:
    default:
        return "Daily";
    }
}

static uint8_t normalize_edit_repeat(uint8_t r)
{
    if (r == CHRONE_ALARM_REPEAT_WORKDAY || r == CHRONE_ALARM_REPEAT_ONCE) {
        return r;
    }
    return CHRONE_ALARM_REPEAT_DAILY;
}

static void refresh_repeat_label(void)
{
    if (s_lbl_repeat) {
        lv_label_set_text_fmt(s_lbl_repeat, "Repeat: %s", repeat_str(s_edit_repeat));
    }
}

static void refresh_rings_at_label(void)
{
    if (!s_lbl_rings_at) {
        return;
    }
    uint8_t h = 0;
    uint8_t m = 0;
    s_time_picker.get_time(&h, &m);
    lv_obj_set_style_text_color(s_lbl_rings_at, lv_color_hex(0x7A8499), 0);
    lv_label_set_text_fmt(s_lbl_rings_at, "Rings at %02u:%02u (24h)", h, m);
}

static void dismiss_error_toast(lv_timer_t *timer)
{
    if (s_error_toast) {
        lv_obj_delete(s_error_toast);
        s_error_toast = nullptr;
    }
    if (timer) {
        lv_timer_delete(timer);
    }
    s_error_toast_timer = nullptr;
}

static void show_error_toast(const char *message)
{
    if (!s_screen || !message) {
        return;
    }

    if (s_error_toast_timer) {
        lv_timer_delete(s_error_toast_timer);
        s_error_toast_timer = nullptr;
    }
    if (s_error_toast) {
        lv_obj_delete(s_error_toast);
        s_error_toast = nullptr;
    }

    s_error_toast = chrone_ui_cont_create(s_screen);
    lv_obj_set_size(s_error_toast, 272, 76);
    lv_obj_align(s_error_toast, LV_ALIGN_CENTER, 0, -8);
    lv_obj_set_style_bg_color(s_error_toast, lv_color_hex(0x1E1018), 0);
    lv_obj_set_style_border_color(s_error_toast, lv_color_hex(0xFF5028), 0);
    lv_obj_set_style_border_width(s_error_toast, 2, 0);
    lv_obj_set_style_radius(s_error_toast, 8, 0);
    lv_obj_set_style_pad_all(s_error_toast, 10, 0);

    lv_obj_t *lbl = lv_label_create(s_error_toast);
    lv_label_set_text(lbl, message);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, 248);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFE8E0), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl);

    lv_obj_move_foreground(s_error_toast);

    s_error_toast_timer = lv_timer_create(dismiss_error_toast, ALARM_ERROR_TOAST_MS, nullptr);
    if (s_error_toast_timer) {
        lv_timer_set_repeat_count(s_error_toast_timer, 1);
    }
}

static void show_once_time_error(void)
{
    if (s_lbl_rings_at) {
        lv_obj_set_style_text_color(s_lbl_rings_at, lv_color_hex(0xFF5028), 0);
        lv_label_set_text(s_lbl_rings_at, "Time too soon (need +2 min)");
    }
    show_error_toast("Once alarm must be\nat least 2 min ahead");
}

static void show_list_panel(void);
static void show_edit_panel(int index);

static bool alarm_slot_is_unset(const chrone_alarm_t *a)
{
    return a && !a->enabled && a->hour == 0 && a->minute == 0;
}

static void save_edit_and_back(void)
{
    if (s_edit_index < 0 || s_edit_index >= CHRONE_ALARM_MAX) {
        show_list_panel();
        return;
    }

    chrone_alarm_t a = {};
    chrone_alarm_get((uint8_t)s_edit_index, &a);
    s_time_picker.get_time(&a.hour, &a.minute);
    if (s_sw_enabled) {
        a.enabled = lv_obj_has_state(s_sw_enabled, LV_STATE_CHECKED);
    }
    a.repeat = s_edit_repeat;
    if (a.enabled && a.repeat == CHRONE_ALARM_REPEAT_ONCE
        && !chrone_alarm_once_time_ok(a.hour, a.minute)) {
        show_once_time_error();
        ESP_LOGW(TAG, "Once alarm save rejected: < %d min ahead", CHRONE_ALARM_ONCE_MIN_LEAD_MIN);
        return;
    }
    if (chrone_alarm_set((uint8_t)s_edit_index, &a) != ESP_OK) {
        show_once_time_error();
        return;
    }
    chrone_alarm_save();
    chrone_haptic_confirm();
    ESP_LOGI(TAG, "saved alarm[%d] %02u:%02u %s", s_edit_index, a.hour, a.minute,
             a.enabled ? "ON" : "off");
    show_list_panel();
}

struct PendingEdit {
    int index;
};

static void deferred_edit_cb(lv_timer_t *timer)
{
    auto *pending = static_cast<PendingEdit *>(lv_timer_get_user_data(timer));
    lv_timer_delete(timer);
    if (!pending) {
        return;
    }
    const int idx = pending->index;
    lv_free(pending);
    show_edit_panel(idx);
}

static void row_click_event(lv_event_t *e)
{
    chrone_haptic_confirm();
    const int idx = (int)(intptr_t)lv_event_get_user_data(e);
    auto *pending = static_cast<PendingEdit *>(lv_malloc(sizeof(PendingEdit)));
    if (!pending) {
        return;
    }
    pending->index = idx;
    lv_timer_t *t = lv_timer_create(deferred_edit_cb, 1, pending);
    if (t) {
        lv_timer_set_repeat_count(t, 1);
    } else {
        lv_free(pending);
    }
}

static void repeat_cycle_event(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    if (s_edit_repeat == CHRONE_ALARM_REPEAT_DAILY) {
        s_edit_repeat = CHRONE_ALARM_REPEAT_WORKDAY;
    } else if (s_edit_repeat == CHRONE_ALARM_REPEAT_WORKDAY) {
        s_edit_repeat = CHRONE_ALARM_REPEAT_ONCE;
    } else {
        s_edit_repeat = CHRONE_ALARM_REPEAT_DAILY;
    }
    refresh_repeat_label();
    chrone_haptic_detent();
}

static lv_obj_t *make_btn(lv_obj_t *parent, const char *txt, lv_event_cb_t cb, void *ud)
{
    lv_obj_t *b = lv_button_create(parent);
    chrone_ui_no_scroll(b);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_center(l);
    if (cb) {
        lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
    }
    return b;
}

static void create_alarm_header(lv_obj_t *screen, const char *title_text)
{
    s_header_bar = chrone_ui_cont_create(screen);
    lv_obj_set_size(s_header_bar, CHRONE_LCD_W, CHRONE_ALARM_HEADER_H);
    lv_obj_align(s_header_bar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(s_header_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_header_bar, 0, 0);
    lv_obj_set_style_pad_all(s_header_bar, 0, 0);
    lv_obj_remove_flag(s_header_bar, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *title = lv_label_create(s_header_bar);
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(title, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_move_foreground(s_header_bar);
}

static void rebuild_list_rows(void)
{
    if (!s_list_panel) {
        return;
    }
    const uint32_t n = lv_obj_get_child_count(s_list_panel);
    for (uint32_t i = n; i > 0; --i) {
        lv_obj_delete(lv_obj_get_child(s_list_panel, i - 1));
    }

    for (int i = 0; i < CHRONE_ALARM_MAX; ++i) {
        chrone_alarm_t a = {};
        chrone_alarm_get((uint8_t)i, &a);

        char buf[48];
        std::snprintf(buf, sizeof(buf), "%s  %02u:%02u  %s",
                      a.enabled ? "[ON]" : "[--]", a.hour, a.minute, repeat_str(a.repeat));

        lv_obj_t *row = lv_button_create(s_list_panel);
        chrone_ui_no_scroll(row);
        lv_obj_set_width(row, CHRONE_LCD_W - 24);
        lv_obj_set_height(row, 36);
        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_22, 0);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(row, row_click_event, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}

static void clear_screen_children(void)
{
    if (!s_screen) {
        return;
    }
    s_time_picker.destroy();
    const uint32_t n = lv_obj_get_child_count(s_screen);
    for (uint32_t i = n; i > 0; --i) {
        lv_obj_delete(lv_obj_get_child(s_screen, i - 1));
    }
    s_list_panel = nullptr;
    s_edit_panel = nullptr;
    s_sw_enabled = nullptr;
    s_lbl_repeat = nullptr;
    s_lbl_rings_at = nullptr;
    s_header_bar = nullptr;
}

static void build_list_ui(void)
{
    s_list_panel = chrone_ui_cont_create(s_screen);
    lv_obj_set_size(s_list_panel, CHRONE_LCD_W - 16, CHRONE_LCD_H - CHRONE_ALARM_HEADER_H - 8);
    lv_obj_align(s_list_panel, LV_ALIGN_TOP_MID, 0, CHRONE_ALARM_HEADER_H + 4);
    lv_obj_set_flex_flow(s_list_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_list_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_list_panel, 6, 0);
    lv_obj_set_style_bg_opa(s_list_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_list_panel, 0, 0);

    rebuild_list_rows();
    create_alarm_header(s_screen, "Alarms");
}

static void build_edit_ui(int index)
{
    s_edit_index = index;
    chrone_alarm_t a = {};
    chrone_alarm_get((uint8_t)index, &a);
    s_edit_repeat = normalize_edit_repeat(a.repeat);

    char title_buf[24];
    std::snprintf(title_buf, sizeof(title_buf), "Alarm #%d", index + 1);

    s_time_picker.create(s_screen);
    if (alarm_slot_is_unset(&a)) {
        s_time_picker.set_default_now_plus_one();
    } else {
        s_time_picker.set_time(a.hour, a.minute);
    }

    s_lbl_rings_at = lv_label_create(s_screen);
    lv_obj_set_style_text_color(s_lbl_rings_at, lv_color_hex(0x7A8499), 0);
    lv_obj_set_style_text_font(s_lbl_rings_at, &lv_font_montserrat_14, 0);
    lv_obj_align(s_lbl_rings_at, LV_ALIGN_TOP_MID, 0, CHRONE_ALARM_META_Y);
    refresh_rings_at_label();

    s_edit_panel = chrone_ui_cont_create(s_screen);
    lv_obj_set_size(s_edit_panel, CHRONE_LCD_W - 16, CHRONE_LCD_H - CHRONE_ALARM_META_Y - 36);
    lv_obj_align(s_edit_panel, LV_ALIGN_TOP_MID, 0, CHRONE_ALARM_META_Y + 18);
    lv_obj_set_style_bg_opa(s_edit_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_edit_panel, 0, 0);
    lv_obj_set_flex_flow(s_edit_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_edit_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_edit_panel, 10, 0);

    lv_obj_t *row_en = chrone_ui_cont_create(s_edit_panel);
    lv_obj_set_size(row_en, LV_PCT(100), 36);
    lv_obj_set_style_bg_opa(row_en, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_en, 0, 0);
    lv_obj_t *len = lv_label_create(row_en);
    lv_label_set_text(len, "Enabled");
    lv_obj_set_style_text_font(len, &lv_font_montserrat_14, 0);
    s_sw_enabled = lv_switch_create(row_en);
    chrone_ui_no_scroll(s_sw_enabled);
    lv_obj_align(len, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_align(s_sw_enabled, LV_ALIGN_RIGHT_MID, -4, 0);
    if (a.enabled || alarm_slot_is_unset(&a)) {
        lv_obj_add_state(s_sw_enabled, LV_STATE_CHECKED);
    }

    lv_obj_t *row_repeat = chrone_ui_cont_create(s_edit_panel);
    lv_obj_set_size(row_repeat, LV_PCT(100), 36);
    lv_obj_set_style_bg_opa(row_repeat, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_repeat, 0, 0);
    lv_obj_add_flag(row_repeat, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row_repeat, repeat_cycle_event, LV_EVENT_CLICKED, nullptr);

    s_lbl_repeat = lv_label_create(row_repeat);
    lv_obj_set_style_text_font(s_lbl_repeat, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_repeat, lv_color_hex(0xD8DCE8), 0);
    lv_obj_align(s_lbl_repeat, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_add_flag(s_lbl_repeat, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_lbl_repeat, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(s_lbl_repeat, repeat_cycle_event, LV_EVENT_CLICKED, nullptr);
    refresh_repeat_label();

    create_alarm_header(s_screen, title_buf);
}

static void show_list_panel(void)
{
    s_edit_index = -1;
    clear_screen_children();
    if (s_screen) {
        chrone_ui_prepare_screen(s_screen);
    }
    build_list_ui();
}

static void show_edit_panel(int index)
{
    if (index < 0 || index >= CHRONE_ALARM_MAX) {
        return;
    }
    clear_screen_children();
    if (s_screen) {
        chrone_ui_prepare_screen(s_screen);
    }
    build_edit_ui(index);
}

extern "C" bool chrone_ui_alarm_config_active(void)
{
    return s_alarm_ui_active;
}

extern "C" void chrone_ui_show_alarm_config(lv_obj_t *screen)
{
    if (!screen) {
        return;
    }
    chrone_ui_pause_clock();
    s_screen = screen;
    s_alarm_ui_active = true;
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x06080C), 0);
    clear_screen_children();
    chrone_ui_prepare_screen(screen);
    chrone_haptic_prepare();
    build_list_ui();
    ESP_LOGI(TAG, "alarm config UI (DSEG picker)");
}

extern "C" void chrone_ui_alarm_config_leave(void)
{
    s_alarm_ui_active = false;
    s_edit_index = -1;
}

extern "C" void chrone_ui_alarm_nav_back(void)
{
    chrone_display_idle_on_touch();
    if (s_edit_index >= 0) {
        save_edit_and_back();
        return;
    }
    chrone_haptic_confirm();
    if (s_screen) {
        chrone_ui_show_settings_hub(s_screen);
    }
}

extern "C" void chrone_ui_show_clock(void)
{
    chrone_ui_alarm_config_leave();

    lv_obj_t *screen = s_screen;
    if (!screen) {
        screen = lv_scr_act();
    }

    chrone_ui_settings_leave();

    if (!screen) {
        ESP_LOGW(TAG, "show_clock: no active screen");
        return;
    }

    s_screen = screen;
    clear_screen_children();
    if (chrone_ui_resume_clock(screen) != ESP_OK) {
        ESP_LOGW(TAG, "show_clock: resume_clock failed");
    }
}
