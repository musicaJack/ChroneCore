#include "chrone_ui.h"
#include "chrone_ui_layout.h"

#include "chrone_alarm.h"
#include "chrone_audio.h"
#include "chrone_display_idle.h"
#include "chrone_haptic.h"
#include "chrone_settings.h"
#include "clock_layout.h"

#include "chrone_hal.h"
#include "esp_log.h"
#include <cstdio>
#include <cstring>

static const char *TAG = "chrone_ui_settings";

static lv_obj_t *s_screen;
static bool s_settings_ui_active;

typedef enum {
    CHRONE_SETTINGS_VIEW_HUB = 0,
    CHRONE_SETTINGS_VIEW_DISPLAY,
    CHRONE_SETTINGS_VIEW_SOUND,
} chrone_settings_view_t;

static chrone_settings_view_t s_settings_view;

static lv_color_t ui_muted(void)
{
    return lv_color_hex(0x7A8499);
}

static void clear_screen_children(lv_obj_t *screen)
{
    if (!screen) {
        return;
    }
    const uint32_t n = lv_obj_get_child_count(screen);
    for (uint32_t i = n; i > 0; --i) {
        lv_obj_delete(lv_obj_get_child(screen, i - 1));
    }
}

static void settings_touch_cb(lv_event_t *e)
{
    (void)e;
    chrone_display_idle_on_touch();
}

static void attach_touch_reset(lv_obj_t *obj)
{
    lv_obj_add_event_cb(obj, settings_touch_cb, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(obj, settings_touch_cb, LV_EVENT_CLICKED, nullptr);
}

static lv_obj_t *create_header(lv_obj_t *screen, const char *title)
{
    lv_obj_t *bar = chrone_ui_cont_create(screen);
    lv_obj_set_size(bar, CHRONE_LCD_W, CHRONE_ALARM_HEADER_H);
    lv_obj_align(bar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *tl = lv_label_create(bar);
    lv_label_set_text(tl, title);
    lv_obj_set_style_text_font(tl, &lv_font_montserrat_14, 0);
    lv_obj_align(tl, LV_ALIGN_CENTER, 0, 0);

    return bar;
}

static lv_obj_t *create_nav_row(lv_obj_t *parent, const char *name, const char *summary,
                                lv_event_cb_t click_cb)
{
    lv_obj_t *row = lv_button_create(parent);
    chrone_ui_no_scroll(row);
    lv_obj_set_width(row, CHRONE_LCD_W - 16);
    lv_obj_set_height(row, 42);
    lv_obj_set_style_pad_hor(row, 8, 0);

    lv_obj_t *left = lv_label_create(row);
    lv_label_set_text(left, name);
    lv_obj_set_style_text_font(left, &lv_font_montserrat_14, 0);
    lv_obj_align(left, LV_ALIGN_LEFT_MID, 4, 0);

    if (summary && summary[0]) {
        lv_obj_t *mid = lv_label_create(row);
        lv_label_set_text(mid, summary);
        lv_obj_set_style_text_color(mid, ui_muted(), 0);
        lv_obj_set_style_text_font(mid, &lv_font_montserrat_14, 0);
        lv_obj_align(mid, LV_ALIGN_RIGHT_MID, -18, 0);
    }

    lv_obj_t *arrow = lv_label_create(row);
    lv_label_set_text(arrow, ">");
    lv_obj_set_style_text_color(arrow, ui_muted(), 0);
    lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -4, 0);

    if (click_cb) {
        lv_obj_add_event_cb(row, click_cb, LV_EVENT_CLICKED, nullptr);
    }
    attach_touch_reset(row);
    return row;
}

static void hub_alarms_event(lv_event_t *e)
{
    (void)e;
    chrone_haptic_confirm();
    if (s_screen) {
        chrone_ui_show_alarm_config(s_screen);
    }
}

static void hub_display_event(lv_event_t *e);
static void hub_sound_event(lv_event_t *e);

static void format_display_summary(char *buf, size_t len)
{
    std::snprintf(buf, len, "%u%% · %lumin",
                  chrone_settings_get_brightness(),
                  (unsigned long)(chrone_settings_get_blank_timeout_s() / 60U));
}

static void format_sound_summary(char *buf, size_t len)
{
    std::snprintf(buf, len, "%u%% · %s",
                  chrone_settings_get_alarm_vol(),
                  chrone_settings_vibe_label(chrone_settings_get_vibe_level()));
}

static void format_alarms_summary(char *buf, size_t len)
{
    const uint8_t n = chrone_alarm_enabled_count();
    if (n == 0) {
        std::snprintf(buf, len, "None");
    } else if (n == 1) {
        std::snprintf(buf, len, "1 enabled");
    } else {
        std::snprintf(buf, len, "%u enabled", n);
    }
}

static void build_settings_hub(void)
{
    s_settings_view = CHRONE_SETTINGS_VIEW_HUB;
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x06080C), 0);
    clear_screen_children(s_screen);
    chrone_ui_prepare_screen(s_screen);

    (void)create_header(s_screen, "Settings");

    lv_obj_t *list = chrone_ui_cont_create(s_screen);
    lv_obj_set_size(list, CHRONE_LCD_W - 8, CHRONE_LCD_H - CHRONE_ALARM_HEADER_H - 8);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, CHRONE_ALARM_HEADER_H + 4);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(list, 6, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);

    char sum[32];
    format_alarms_summary(sum, sizeof(sum));
    create_nav_row(list, "Alarms", sum, hub_alarms_event);

    format_display_summary(sum, sizeof(sum));
    create_nav_row(list, "Display", sum, hub_display_event);

    format_sound_summary(sum, sizeof(sum));
    create_nav_row(list, "Sound & vibration", sum, hub_sound_event);
}

/* -------- Display sub-page -------- */

static const uint32_t k_blank_presets_s[] = {60, 120, 180, 300, 600};

static void blank_btn_event(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    const size_t idx = (size_t)(intptr_t)lv_event_get_user_data(e);
    if (idx >= sizeof(k_blank_presets_s) / sizeof(k_blank_presets_s[0])) {
        return;
    }
    chrone_haptic_confirm();
    (void)chrone_settings_set_blank_timeout_s(k_blank_presets_s[idx]);
    chrone_display_idle_on_touch();
}

static void brightness_slider_event(lv_event_t *e)
{
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    if (!slider) {
        return;
    }
    const int32_t v = lv_slider_get_value(slider);
    static int32_t s_last = -1;
    if (v == s_last) {
        return;
    }
    s_last = v;
    chrone_haptic_detent();
    (void)chrone_settings_set_brightness((uint8_t)v);
    (void)chrone_hal_set_brightness((uint8_t)v);
    chrone_display_idle_on_touch();
}

static void build_display_page(void)
{
    s_settings_view = CHRONE_SETTINGS_VIEW_DISPLAY;
    clear_screen_children(s_screen);
    chrone_ui_prepare_screen(s_screen);
    (void)create_header(s_screen, "Display");

    lv_obj_t *panel = chrone_ui_cont_create(s_screen);
    lv_obj_set_size(panel, CHRONE_LCD_W - 16, CHRONE_LCD_H - CHRONE_ALARM_HEADER_H - 12);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, CHRONE_ALARM_HEADER_H + 6);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel, 0, 0);

    lv_obj_t *lbl_b = lv_label_create(panel);
    lv_label_set_text(lbl_b, "Brightness");
    lv_obj_set_style_text_font(lbl_b, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_b, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *slider = lv_slider_create(panel);
    chrone_ui_no_scroll(slider);
    lv_obj_set_width(slider, CHRONE_LCD_W - 40);
    lv_slider_set_range(slider, CHRONE_SETTINGS_BRIGHTNESS_MIN, CHRONE_SETTINGS_BRIGHTNESS_MAX);
    lv_slider_set_value(slider, chrone_settings_get_brightness(), LV_ANIM_OFF);
    lv_obj_align(slider, LV_ALIGN_TOP_LEFT, 0, 22);
    lv_obj_add_event_cb(slider, brightness_slider_event, LV_EVENT_VALUE_CHANGED, nullptr);
    attach_touch_reset(slider);

    char pct[8];
    std::snprintf(pct, sizeof(pct), "%u%%", chrone_settings_get_brightness());
    lv_obj_t *lbl_pct = lv_label_create(panel);
    lv_label_set_text(lbl_pct, pct);
    lv_obj_set_style_text_color(lbl_pct, ui_muted(), 0);
    lv_obj_align(lbl_pct, LV_ALIGN_TOP_RIGHT, 0, 0);

    lv_obj_t *lbl_t = lv_label_create(panel);
    lv_label_set_text(lbl_t, "Turn off display after");
    lv_obj_set_style_text_font(lbl_t, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_t, LV_ALIGN_TOP_LEFT, 0, 52);

    lv_obj_t *row = chrone_ui_cont_create(panel);
    lv_obj_set_size(row, CHRONE_LCD_W - 32, 36);
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, 72);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_column(row, 4, 0);

    const uint32_t cur = chrone_settings_get_blank_timeout_s();
    static const char *labels[] = {"1", "2", "3", "5", "10"};
    for (size_t i = 0; i < 5; ++i) {
        lv_obj_t *b = lv_button_create(row);
        chrone_ui_no_scroll(b);
        lv_obj_set_size(b, 44, 32);
        lv_obj_t *bl = lv_label_create(b);
        lv_label_set_text(bl, labels[i]);
        lv_obj_center(bl);
        if (cur == k_blank_presets_s[i]) {
            lv_obj_set_style_bg_color(b, lv_color_hex(0x2A3550), 0);
        }
        lv_obj_add_event_cb(b, blank_btn_event, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        attach_touch_reset(b);
    }

    lv_obj_t *hint = lv_label_create(panel);
    lv_label_set_text(hint, "No touch -> backlight off");
    lv_obj_set_style_text_color(hint, ui_muted(), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_LEFT, 0, 114);
}

static void hub_display_event(lv_event_t *e)
{
    (void)e;
    chrone_haptic_confirm();
    build_display_page();
}

/* -------- Sound & vibration -------- */

static void vol_slider_event(lv_event_t *e)
{
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    if (!slider) {
        return;
    }
    const int32_t v = lv_slider_get_value(slider);
    static int32_t s_last = -1;
    if (v == s_last) {
        return;
    }
    s_last = v;
    chrone_haptic_detent();
    (void)chrone_settings_set_alarm_vol((uint8_t)v);
}

static lv_timer_t *s_preview_stop_timer;

static void preview_stop_cb(lv_timer_t *t)
{
    (void)t;
    chrone_audio_alarm_stop();
    s_preview_stop_timer = nullptr;
}

static void preview_event_full(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    chrone_haptic_confirm();

    if (s_preview_stop_timer) {
        lv_timer_delete(s_preview_stop_timer);
        s_preview_stop_timer = nullptr;
    }
    chrone_audio_alarm_stop();

    chrone_audio_alarm_start();
    s_preview_stop_timer = lv_timer_create(preview_stop_cb, 2000, nullptr);
    if (s_preview_stop_timer) {
        lv_timer_set_repeat_count(s_preview_stop_timer, 1);
    }
}

static void vibe_btn_event(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    const uint8_t level = (uint8_t)(intptr_t)lv_event_get_user_data(e);
    chrone_haptic_confirm();
    (void)chrone_settings_set_vibe_level(level);
    chrone_haptic_vibe_preview(level);
}

static void build_sound_page(void)
{
    s_settings_view = CHRONE_SETTINGS_VIEW_SOUND;
    clear_screen_children(s_screen);
    chrone_ui_prepare_screen(s_screen);
    (void)create_header(s_screen, "Sound & vibration");

    lv_obj_t *panel = chrone_ui_cont_create(s_screen);
    lv_obj_set_size(panel, CHRONE_LCD_W - 16, CHRONE_LCD_H - CHRONE_ALARM_HEADER_H - 12);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, CHRONE_ALARM_HEADER_H + 6);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel, 0, 0);

    lv_obj_t *lbl_v = lv_label_create(panel);
    lv_label_set_text(lbl_v, "Alarm volume");
    lv_obj_align(lbl_v, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *slider = lv_slider_create(panel);
    chrone_ui_no_scroll(slider);
    lv_obj_set_width(slider, CHRONE_LCD_W - 80);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, chrone_settings_get_alarm_vol(), LV_ANIM_OFF);
    lv_obj_align(slider, LV_ALIGN_TOP_LEFT, 0, 20);
    lv_obj_add_event_cb(slider, vol_slider_event, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t *prev = lv_button_create(panel);
    chrone_ui_no_scroll(prev);
    lv_obj_set_size(prev, 64, 32);
    lv_obj_align(prev, LV_ALIGN_TOP_RIGHT, 0, 14);
    lv_obj_t *pl = lv_label_create(prev);
    lv_label_set_text(pl, "Preview");
    lv_obj_center(pl);
    lv_obj_add_event_cb(prev, preview_event_full, LV_EVENT_CLICKED, nullptr);
    attach_touch_reset(prev);

    lv_obj_t *lbl_z = lv_label_create(panel);
    lv_label_set_text(lbl_z, "Vibration");
    lv_obj_align(lbl_z, LV_ALIGN_TOP_LEFT, 0, 58);

    static const char *tags[] = {"Off", "Weak", "Med", "Strong"};
    const uint8_t cur = chrone_settings_get_vibe_level();
    lv_obj_t *row = chrone_ui_cont_create(panel);
    lv_obj_set_size(row, CHRONE_LCD_W - 32, 36);
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, 78);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);

    for (uint8_t i = 0; i <= CHRONE_SETTINGS_VIBE_LEVEL_MAX; ++i) {
        lv_obj_t *b = lv_button_create(row);
        chrone_ui_no_scroll(b);
        lv_obj_set_size(b, 68, 32);
        lv_obj_t *bl = lv_label_create(b);
        lv_label_set_text(bl, tags[i]);
        lv_obj_center(bl);
        if (i == cur) {
            lv_obj_set_style_bg_color(b, lv_color_hex(0x2A3550), 0);
        }
        lv_obj_add_event_cb(b, vibe_btn_event, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        attach_touch_reset(b);
    }
}

static void hub_sound_event(lv_event_t *e)
{
    (void)e;
    chrone_haptic_confirm();
    build_sound_page();
}

extern "C" void chrone_ui_settings_leave(void)
{
    s_settings_ui_active = false;
}

extern "C" void chrone_ui_nav_back(void)
{
    if (!chrone_ui_in_settings_tree()) {
        return;
    }

    if (chrone_ui_alarm_config_active()) {
        chrone_ui_alarm_nav_back();
        return;
    }

    chrone_haptic_confirm();
    chrone_display_idle_on_touch();

    switch (s_settings_view) {
    case CHRONE_SETTINGS_VIEW_DISPLAY:
    case CHRONE_SETTINGS_VIEW_SOUND:
        build_settings_hub();
        break;
    case CHRONE_SETTINGS_VIEW_HUB:
    default:
        chrone_ui_show_clock();
        break;
    }
}

extern "C" void chrone_ui_show_settings_hub(lv_obj_t *screen)
{
    if (!screen) {
        return;
    }
    chrone_ui_pause_clock();
    chrone_ui_alarm_config_leave();
    s_screen = screen;
    s_settings_ui_active = true;
    chrone_display_idle_wake();
    build_settings_hub();
    ESP_LOGI(TAG, "Settings hub");
}

extern "C" bool chrone_ui_settings_active(void)
{
    return s_settings_ui_active;
}

extern "C" bool chrone_ui_in_settings_tree(void)
{
    return s_settings_ui_active || chrone_ui_alarm_config_active();
}
