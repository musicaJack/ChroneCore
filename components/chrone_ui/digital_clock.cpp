#include "chrone_ui.h"

#include "analog_clock.hpp"
#include "chrone_alarm.h"
#include "chrone_alarm_icon.h"
#include "chrone_alarm_icon_bitmap.h"
#include "chrone_input.h"
#include "chrone_font_dseg56.h"
#include "chrone_time.h"
#include "chrone_weather.h"
#include "chrone_weather_icon.h"
#include "weather_code_map.h"
#include "chrone_wifi.h"
#include "clock_layout.h"

#include "esp_log.h"
#include <time.h>
#include "nvs_flash.h"
#include "nvs.h"
#include <cstdio>
#include <cstring>

static const char *TAG = "chrone_ui";
static const char *NVS_NS = "chrone";
static const char *NVS_KEY_MODE = "clk_mode";

static lv_obj_t *s_screen;
static lv_obj_t *s_tap_layer;
static lv_obj_t *s_label_date;
static lv_obj_t *s_label_week;
static lv_obj_t *s_alarm_icon_img;
static uint16_t s_alarm_icon_buf[CHRONE_ALARM_ICON_SZ * CHRONE_ALARM_ICON_SZ];
static lv_image_dsc_t s_alarm_icon_dsc;
static bool s_prev_alarm_indicator;
static lv_obj_t *s_footer_row;
static lv_obj_t *s_weather_icon_img;
static lv_obj_t *s_label_weather_text;
static lv_obj_t *s_label_temp;
static lv_obj_t *s_label_city;
static uint16_t s_weather_icon_buf[CHRONE_WEATHER_ICON_SZ * CHRONE_WEATHER_ICON_SZ];
static lv_image_dsc_t s_weather_icon_dsc;
static int s_prev_weather_code = -2;
static bool s_prev_is_day = true;
static bool s_prov_ui = false;
static lv_obj_t *s_time_row;
static lv_obj_t *s_lb_hh;
static lv_obj_t *s_lb_c1;
static lv_obj_t *s_lb_mm;
static lv_obj_t *s_lb_c2;
static lv_obj_t *s_lb_ss;
static lv_timer_t *s_tick_timer;
static lv_obj_t *s_ring_label;
static lv_timer_t *s_alarm_icon_blink_timer;
static bool s_alarm_icon_blink_on;

#define ALARM_ICON_BLINK_MS    380
#define RING_TAP_HINT_Y_OFFSET 50
static chrone::ui::AnalogClockView s_analog;
static chrone_ui_mode_t s_mode = CHRONE_UI_MODE_DIGITAL;

static int s_prev_hour = -1;
static int s_prev_min = -1;
static int s_prev_sec = -1;
static int s_prev_year = -1;
static int s_prev_mon = -1;
static int s_prev_mday = -1;
static int s_prev_wday = -1;
static uint32_t s_last_tap_ms;

/** 长按进入闹钟配置（ms） */
#define CLOCK_LONG_PRESS_MS 3000
static lv_timer_t *s_alarm_long_press_timer;
static bool s_suppress_tap_click;

static void release_clock_handles(void);
static void clear_screen_children(lv_obj_t *screen);
static esp_err_t rebuild_clock_screen(lv_obj_t *screen);

static lv_color_t ui_color_bg(void) { return lv_color_hex(0x06080C); }
static lv_color_t ui_color_header(void) { return lv_color_hex(0xD8DCE8); }
static lv_color_t ui_color_muted(void) { return lv_color_hex(0x7A8499); }
static lv_color_t ui_color_seg_on(void) { return lv_color_hex(0xFFB020); }
static lv_color_t ui_color_alarm_icon(void) { return lv_color_hex(0xFFB020); }
/** 响铃闪烁：更亮的橙红色 */
static lv_color_t ui_color_alarm_icon_ring(void) { return lv_color_hex(0xFF5028); }

static void load_mode_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) {
        return;
    }
    uint8_t v = CHRONE_UI_MODE_DIGITAL;
    if (nvs_get_u8(h, NVS_KEY_MODE, &v) == ESP_OK && v <= CHRONE_UI_MODE_ANALOG) {
        s_mode = static_cast<chrone_ui_mode_t>(v);
    }
    nvs_close(h);
}

static esp_err_t save_mode_to_nvs(chrone_ui_mode_t mode)
{
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_set_u8(h, NVS_KEY_MODE, static_cast<uint8_t>(mode));
    if (ret == ESP_OK) {
        ret = nvs_commit(h);
    }
    nvs_close(h);
    return ret;
}

static void style_seg_label(lv_obj_t *label)
{
    lv_obj_set_style_text_font(label, &chrone_font_dseg56, 0);
    lv_obj_set_style_text_color(label, ui_color_seg_on(), 0);
    lv_obj_set_style_pad_all(label, 0, 0);
    lv_obj_set_style_border_width(label, 0, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
}

static void invalidate_label(lv_obj_t *label)
{
    if (label) {
        lv_obj_invalidate(label);
    }
}

static void update_time_parts(const struct tm *tm_local, bool force_all)
{
    if (!tm_local || s_mode != CHRONE_UI_MODE_DIGITAL) {
        return;
    }

    if (force_all || tm_local->tm_hour != s_prev_hour) {
        if (s_lb_hh) {
            lv_label_set_text_fmt(s_lb_hh, "%02d", tm_local->tm_hour);
            invalidate_label(s_lb_hh);
        }
        s_prev_hour = tm_local->tm_hour;
    }

    if (force_all || tm_local->tm_min != s_prev_min) {
        if (s_lb_mm) {
            lv_label_set_text_fmt(s_lb_mm, "%02d", tm_local->tm_min);
            invalidate_label(s_lb_mm);
        }
        s_prev_min = tm_local->tm_min;
    }

    if (force_all || tm_local->tm_sec != s_prev_sec) {
        if (s_lb_ss) {
            lv_label_set_text_fmt(s_lb_ss, "%02d", tm_local->tm_sec);
            invalidate_label(s_lb_ss);
        }
        s_prev_sec = tm_local->tm_sec;
    }
}

static void update_header(const struct tm *tm_local, bool force_all)
{
    if (!tm_local) {
        return;
    }

    const int year = tm_local->tm_year + 1900;
    const int mon = tm_local->tm_mon + 1;
    const int mday = tm_local->tm_mday;
    const int wday = tm_local->tm_wday;

    if (force_all || year != s_prev_year || mon != s_prev_mon || mday != s_prev_mday) {
        char date_buf[32];
        std::snprintf(date_buf, sizeof(date_buf), "%04d-%02d-%02d", year, mon, mday);
        if (s_label_date) {
            lv_label_set_text(s_label_date, date_buf);
            invalidate_label(s_label_date);
        }
        s_prev_year = year;
        s_prev_mon = mon;
        s_prev_mday = mday;
    }

    if (force_all || wday != s_prev_wday) {
        if (s_label_week) {
            lv_label_set_text(s_label_week, chrone_time_weekday_en(wday));
            invalidate_label(s_label_week);
        }
        s_prev_wday = wday;
    }
}

static void reset_tick_cache(void)
{
    s_prev_hour = -1;
    s_prev_min = -1;
    s_prev_sec = -1;
    s_prev_year = -1;
    s_prev_mon = -1;
    s_prev_mday = -1;
    s_prev_wday = -1;
}

static void stop_tick_timer(void)
{
    if (s_tick_timer) {
        lv_timer_delete(s_tick_timer);
        s_tick_timer = nullptr;
    }
}

static bool ui_is_daytime(const struct tm *tm_local)
{
    if (tm_local) {
        return tm_local->tm_hour >= 6 && tm_local->tm_hour < 18;
    }
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    return t.tm_hour >= 6 && t.tm_hour < 18;
}

static uint16_t ui_color_to_rgb565(lv_color_t c)
{
    return lv_color_to_u16(c);
}

static void update_alarm_indicator(void)
{
    if (!s_alarm_icon_img) {
        return;
    }
    if (chrone_alarm_get_state() == CHRONE_ALARM_STATE_RINGING) {
        return;
    }

    const bool on = chrone_alarm_scheduling_enabled();
    if (on == s_prev_alarm_indicator) {
        return;
    }
    s_prev_alarm_indicator = on;

    if (on) {
        const uint16_t fg = ui_color_to_rgb565(ui_color_alarm_icon());
        const uint16_t bg = ui_color_to_rgb565(ui_color_bg());
        chrone_alarm_icon_render_rgb565(s_alarm_icon_buf, fg, bg);
        lv_image_set_src(s_alarm_icon_img, &s_alarm_icon_dsc);
        lv_obj_remove_flag(s_alarm_icon_img, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_alarm_icon_img, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_invalidate(s_alarm_icon_img);
}

static void update_weather_icon(int code, bool is_day)
{
    if (!s_weather_icon_img) {
        return;
    }
    if (code == s_prev_weather_code && is_day == s_prev_is_day) {
        return;
    }
    s_prev_weather_code = code;
    s_prev_is_day = is_day;

    const uint16_t fg = ui_color_to_rgb565(ui_color_header());
    const uint16_t bg = ui_color_to_rgb565(ui_color_bg());
    chrone_weather_icon_render_rgb565(code, is_day, s_weather_icon_buf, fg, bg);
    lv_image_set_src(s_weather_icon_img, &s_weather_icon_dsc);
    if (code >= 0) {
        lv_obj_remove_flag(s_weather_icon_img, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_weather_icon_img, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_invalidate(s_weather_icon_img);
}

static void update_weather_strip(const struct tm *tm_local)
{
    if (!s_footer_row) {
        return;
    }

    weather_info_t *w = chrone_weather_get_info();
    const bool is_day = ui_is_daytime(tm_local);

    if (w && w->available) {
        if (s_label_weather_text) {
            char desc[32];
            if (w->code >= 0) {
                weather_code_get_desc(w->code, is_day, desc, sizeof(desc));
            } else {
                std::snprintf(desc, sizeof(desc), "%s", w->text[0] ? w->text : "--");
            }
            lv_label_set_text(s_label_weather_text, desc);
            invalidate_label(s_label_weather_text);
        }
        if (s_label_temp) {
            char tbuf[16];
            std::snprintf(tbuf, sizeof(tbuf), "%s°C", w->temp);
            lv_label_set_text(s_label_temp, tbuf);
            invalidate_label(s_label_temp);
        }
        if (s_label_city) {
            lv_label_set_text(s_label_city, w->city[0] ? w->city : "--");
            invalidate_label(s_label_city);
        }
        update_weather_icon(w->code, is_day);
    } else if (chrone_wifi_is_connected()) {
        if (s_label_weather_text) {
            lv_label_set_text(s_label_weather_text, "--");
            invalidate_label(s_label_weather_text);
        }
        if (s_label_temp) {
            lv_label_set_text(s_label_temp, "--");
            invalidate_label(s_label_temp);
        }
        if (s_label_city) {
            lv_label_set_text(s_label_city, "--");
            invalidate_label(s_label_city);
        }
        update_weather_icon(-1, is_day);
    } else {
        if (s_label_weather_text) {
            lv_label_set_text(s_label_weather_text, "WiFi...");
            invalidate_label(s_label_weather_text);
        }
        if (s_label_temp) {
            lv_label_set_text(s_label_temp, "");
            invalidate_label(s_label_temp);
        }
        if (s_label_city) {
            lv_label_set_text(s_label_city, "");
            invalidate_label(s_label_city);
        }
        update_weather_icon(-1, is_day);
    }
}

static esp_err_t create_provisioning_screen(lv_obj_t *screen)
{
    char ssid[64];
    char url[64];
    chrone_wifi_get_provisioning_hint(ssid, sizeof(ssid), url, sizeof(url));

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "WiFi Setup");
    lv_obj_set_style_text_color(title, ui_color_header(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_t *hint1 = lv_label_create(screen);
    lv_label_set_text(hint1, "1. Connect phone to hotspot:");
    lv_obj_set_style_text_color(hint1, ui_color_muted(), 0);
    lv_obj_set_style_text_font(hint1, &lv_font_montserrat_14, 0);
    lv_obj_align(hint1, LV_ALIGN_TOP_MID, 0, 56);

    lv_obj_t *lb_ssid = lv_label_create(screen);
    lv_label_set_text(lb_ssid, ssid);
    lv_obj_set_style_text_color(lb_ssid, ui_color_seg_on(), 0);
    lv_obj_set_style_text_font(lb_ssid, &lv_font_montserrat_22, 0);
    lv_obj_align(lb_ssid, LV_ALIGN_TOP_MID, 0, 78);

    lv_obj_t *hint2 = lv_label_create(screen);
    lv_label_set_text(hint2, "2. Open in browser:");
    lv_obj_set_style_text_color(hint2, ui_color_muted(), 0);
    lv_obj_set_style_text_font(hint2, &lv_font_montserrat_14, 0);
    lv_obj_align(hint2, LV_ALIGN_TOP_MID, 0, 110);

    lv_obj_t *lb_url = lv_label_create(screen);
    lv_label_set_text(lb_url, url);
    lv_obj_set_style_text_color(lb_url, ui_color_seg_on(), 0);
    lv_obj_set_style_text_font(lb_url, &lv_font_montserrat_14, 0);
    lv_obj_align(lb_url, LV_ALIGN_TOP_MID, 0, 132);

    lv_obj_t *hint3 = lv_label_create(screen);
    lv_label_set_text(hint3, "Advanced: pick weather city");
    lv_obj_set_style_text_color(hint3, ui_color_muted(), 0);
    lv_obj_set_style_text_font(hint3, &lv_font_montserrat_14, 0);
    lv_obj_align(hint3, LV_ALIGN_TOP_MID, 0, 168);

    return ESP_OK;
}

static void show_provisioning_ui(void)
{
    if (!s_screen || s_prov_ui) {
        return;
    }
    release_clock_handles();
    clear_screen_children(s_screen);
    s_prov_ui = true;
    lv_obj_set_style_bg_color(s_screen, ui_color_bg(), 0);
    create_provisioning_screen(s_screen);
    ESP_LOGI(TAG, "provisioning UI");
}

static void hide_provisioning_ui(void)
{
    if (!s_prov_ui) {
        return;
    }
    s_prov_ui = false;
    rebuild_clock_screen(s_screen);
}

extern "C" void chrone_ui_poll_services(void)
{
    if (chrone_wifi_get_state() == CHRONE_WIFI_STATE_PROVISIONING) {
        show_provisioning_ui();
        return;
    }
    if (s_prov_ui) {
        hide_provisioning_ui();
        return;
    }
    struct tm tm_local;
    if (chrone_time_get_local_tm(&tm_local) == ESP_OK) {
        update_weather_strip(&tm_local);
    }
}

static void tick_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    chrone_ui_poll_services();
    if (s_prov_ui) {
        return;
    }

    struct tm tm_local;
    if (chrone_time_get_local_tm(&tm_local) != ESP_OK) {
        return;
    }

    update_header(&tm_local, false);
    update_alarm_indicator();
    update_weather_strip(&tm_local);
    if (s_mode == CHRONE_UI_MODE_DIGITAL) {
        update_time_parts(&tm_local, false);
    } else if (s_analog.is_ready()) {
        s_analog.on_second_tick(&tm_local);
    }

    if (!chrone_ui_alarm_config_active()) {
        chrone_ui_update_ringing();
    }
}

static void start_tick_timer(void)
{
    stop_tick_timer();
    s_tick_timer = lv_timer_create(tick_timer_cb, 1000, nullptr);
    if (s_tick_timer) {
        lv_timer_set_repeat_count(s_tick_timer, -1);
    }
}

static void style_footer_label(lv_obj_t *label)
{
    lv_obj_set_style_text_color(label, ui_color_muted(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
}

static esp_err_t create_header_labels(lv_obj_t *screen)
{
    s_label_date = lv_label_create(screen);
    lv_obj_set_style_text_color(s_label_date, ui_color_header(), 0);
    lv_obj_set_style_text_font(s_label_date, &lv_font_montserrat_22, 0);
    lv_label_set_text(s_label_date, "----/--/--");
    lv_obj_align(s_label_date, LV_ALIGN_TOP_LEFT, 8, 4);

    s_label_week = lv_label_create(screen);
    lv_obj_set_style_text_color(s_label_week, ui_color_muted(), 0);
    lv_obj_set_style_text_font(s_label_week, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_label_week, "---");
    lv_obj_align(s_label_week, LV_ALIGN_TOP_RIGHT, -8, 8);

    std::memset(&s_alarm_icon_dsc, 0, sizeof(s_alarm_icon_dsc));
    s_alarm_icon_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_alarm_icon_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    s_alarm_icon_dsc.header.w = CHRONE_ALARM_ICON_SZ;
    s_alarm_icon_dsc.header.h = CHRONE_ALARM_ICON_SZ;
    s_alarm_icon_dsc.header.stride = CHRONE_ALARM_ICON_SZ * 2;
    s_alarm_icon_dsc.data_size = sizeof(s_alarm_icon_buf);
    s_alarm_icon_dsc.data = reinterpret_cast<const uint8_t *>(s_alarm_icon_buf);

    s_alarm_icon_img = lv_image_create(screen);
    lv_image_set_src(s_alarm_icon_img, &s_alarm_icon_dsc);
    lv_obj_set_size(s_alarm_icon_img, CHRONE_ALARM_ICON_SZ, CHRONE_ALARM_ICON_SZ);
    lv_obj_align(s_alarm_icon_img, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_add_flag(s_alarm_icon_img, LV_OBJ_FLAG_HIDDEN);
    s_prev_alarm_indicator = false;
    update_alarm_indicator();

    return ESP_OK;
}

static esp_err_t create_footer_bar(lv_obj_t *screen)
{
    s_footer_row = lv_obj_create(screen);
    lv_obj_set_size(s_footer_row, CHRONE_LCD_W - 16, CHRONE_FOOTER_H);
    lv_obj_align(s_footer_row, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_opa(s_footer_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_footer_row, 0, 0);
    lv_obj_set_style_pad_all(s_footer_row, 0, 0);
    lv_obj_set_style_pad_column(s_footer_row, 6, 0);
    lv_obj_remove_flag(s_footer_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_footer_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_flex_flow(s_footer_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_footer_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    std::memset(&s_weather_icon_dsc, 0, sizeof(s_weather_icon_dsc));
    s_weather_icon_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_weather_icon_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    s_weather_icon_dsc.header.w = CHRONE_WEATHER_ICON_SZ;
    s_weather_icon_dsc.header.h = CHRONE_WEATHER_ICON_SZ;
    s_weather_icon_dsc.header.stride = CHRONE_WEATHER_ICON_SZ * 2;
    s_weather_icon_dsc.data_size = sizeof(s_weather_icon_buf);
    s_weather_icon_dsc.data = reinterpret_cast<const uint8_t *>(s_weather_icon_buf);

    s_weather_icon_img = lv_image_create(s_footer_row);
    lv_image_set_src(s_weather_icon_img, &s_weather_icon_dsc);
    lv_obj_set_size(s_weather_icon_img, CHRONE_WEATHER_ICON_SZ, CHRONE_WEATHER_ICON_SZ);
    lv_obj_add_flag(s_weather_icon_img, LV_OBJ_FLAG_HIDDEN);

    s_label_weather_text = lv_label_create(s_footer_row);
    style_footer_label(s_label_weather_text);
    lv_label_set_text(s_label_weather_text, "--");

    s_label_temp = lv_label_create(s_footer_row);
    style_footer_label(s_label_temp);
    lv_label_set_text(s_label_temp, "--");

    lv_obj_t *spacer = lv_obj_create(s_footer_row);
    lv_obj_set_size(spacer, 1, 1);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_remove_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

    s_label_city = lv_label_create(s_footer_row);
    style_footer_label(s_label_city);
    lv_label_set_text(s_label_city, "--");

    s_prev_weather_code = -2;
    s_prev_is_day = true;
    return ESP_OK;
}

static lv_obj_t *create_seg_part(lv_obj_t *parent, const char *text)
{
    lv_obj_t *lb = lv_label_create(parent);
    style_seg_label(lb);
    lv_label_set_text(lb, text);
    return lb;
}

static esp_err_t create_time_row(lv_obj_t *screen)
{
    s_time_row = lv_obj_create(screen);
    lv_obj_set_size(s_time_row, CHRONE_LCD_W, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(s_time_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_time_row, 0, 0);
    lv_obj_set_style_pad_all(s_time_row, 0, 0);
    lv_obj_set_style_pad_column(s_time_row, 4, 0);
    lv_obj_remove_flag(s_time_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_time_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_time_row, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    lv_obj_set_flex_flow(s_time_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_time_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(s_time_row, LV_ALIGN_TOP_MID, 0, CHRONE_TIME_Y);

    s_lb_hh = create_seg_part(s_time_row, "00");
    s_lb_c1 = create_seg_part(s_time_row, ":");
    s_lb_mm = create_seg_part(s_time_row, "00");
    s_lb_c2 = create_seg_part(s_time_row, ":");
    s_lb_ss = create_seg_part(s_time_row, "00");

    if (!s_lb_hh || !s_lb_c1 || !s_lb_mm || !s_lb_c2 || !s_lb_ss) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static esp_err_t create_digital_widgets(lv_obj_t *screen)
{
    esp_err_t ret = create_header_labels(screen);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = create_time_row(screen);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = create_footer_bar(screen);
    if (ret != ESP_OK) {
        return ret;
    }
    reset_tick_cache();
    return ESP_OK;
}

static esp_err_t create_analog_widgets(lv_obj_t *screen)
{
    esp_err_t ret = create_header_labels(screen);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = s_analog.create(screen);
    if (ret != ESP_OK) {
        return ret;
    }
    return create_footer_bar(screen);
}

static void ring_dismiss_event_cb(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_CLICKED && code != LV_EVENT_RELEASED) {
        return;
    }
    if (chrone_alarm_get_state() != CHRONE_ALARM_STATE_RINGING) {
        return;
    }
    chrone_alarm_dismiss();
}

static void stop_alarm_icon_blink_timer(void)
{
    if (s_alarm_icon_blink_timer) {
        lv_timer_delete(s_alarm_icon_blink_timer);
        s_alarm_icon_blink_timer = nullptr;
    }
}

static void alarm_icon_invalidate_local(void)
{
    if (!s_alarm_icon_img) {
        return;
    }
    lv_area_t area;
    lv_obj_get_coords(s_alarm_icon_img, &area);
    lv_obj_invalidate_area(s_alarm_icon_img, &area);
    if (s_screen) {
        lv_obj_invalidate_area(s_screen, &area);
    }
}

/** 局部清除：24×24 缓冲区填表盘底色，仅刷新图标区域 */
static void alarm_icon_clear_local(void)
{
    if (!s_alarm_icon_img) {
        return;
    }
    const uint16_t bg = ui_color_to_rgb565(ui_color_bg());
    const size_t n = CHRONE_ALARM_ICON_SZ * CHRONE_ALARM_ICON_SZ;
    for (size_t i = 0; i < n; ++i) {
        s_alarm_icon_buf[i] = bg;
    }
    lv_image_set_src(s_alarm_icon_img, &s_alarm_icon_dsc);
    lv_obj_remove_flag(s_alarm_icon_img, LV_OBJ_FLAG_HIDDEN);
    alarm_icon_invalidate_local();
}

/** 局部重绘：重新光栅化闹钟图标并只刷新该区域 */
static void alarm_icon_redraw_local(void)
{
    if (!s_alarm_icon_img) {
        return;
    }
    const uint16_t fg = ui_color_to_rgb565(ui_color_alarm_icon_ring());
    const uint16_t bg = ui_color_to_rgb565(ui_color_bg());
    chrone_alarm_icon_render_rgb565(s_alarm_icon_buf, fg, bg);
    lv_image_set_src(s_alarm_icon_img, &s_alarm_icon_dsc);
    lv_obj_remove_flag(s_alarm_icon_img, LV_OBJ_FLAG_HIDDEN);
    alarm_icon_invalidate_local();
}

static void alarm_icon_blink_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!s_alarm_icon_img || chrone_alarm_get_state() != CHRONE_ALARM_STATE_RINGING) {
        return;
    }
    s_alarm_icon_blink_on = !s_alarm_icon_blink_on;
    if (s_alarm_icon_blink_on) {
        alarm_icon_redraw_local();
    } else {
        alarm_icon_clear_local();
    }
}

static void start_alarm_icon_blink(void)
{
    stop_alarm_icon_blink_timer();
    s_alarm_icon_blink_on = true;
    alarm_icon_redraw_local();
    s_alarm_icon_blink_timer = lv_timer_create(alarm_icon_blink_timer_cb, ALARM_ICON_BLINK_MS, nullptr);
    if (s_alarm_icon_blink_timer) {
        lv_timer_set_repeat_count(s_alarm_icon_blink_timer, -1);
    }
}

static void restore_alarm_indicator_after_ring(void)
{
    s_prev_alarm_indicator = !chrone_alarm_scheduling_enabled();
    update_alarm_indicator();
}

static void destroy_ringing_ui(void)
{
    stop_alarm_icon_blink_timer();
    restore_alarm_indicator_after_ring();
    if (s_ring_label) {
        lv_obj_delete(s_ring_label);
        s_ring_label = nullptr;
    }
}

static void create_ringing_hint(lv_obj_t *screen)
{
    if (s_ring_label || !screen) {
        return;
    }

    s_ring_label = lv_label_create(screen);
    lv_label_set_text(s_ring_label, "Tap to stop");
    lv_obj_set_style_text_color(s_ring_label, lv_color_hex(0xFFE0A0), 0);
    lv_obj_set_style_text_align(s_ring_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_ring_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_opa(s_ring_label, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_ring_label, 0, 0);
    lv_obj_align(s_ring_label, LV_ALIGN_CENTER, 0, RING_TAP_HINT_Y_OFFSET);
    lv_obj_add_flag(s_ring_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_ring_label, ring_dismiss_event_cb, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(s_ring_label, ring_dismiss_event_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(s_ring_label, ring_dismiss_event_cb, LV_EVENT_RELEASED, nullptr);
    lv_obj_move_foreground(s_ring_label);
}

extern "C" void chrone_ui_update_ringing(void)
{
    if (!s_screen) {
        return;
    }

    if (chrone_alarm_get_state() == CHRONE_ALARM_STATE_RINGING) {
        create_ringing_hint(s_screen);
        start_alarm_icon_blink();
        if (s_alarm_icon_img) {
            lv_obj_move_foreground(s_alarm_icon_img);
        }
        if (s_ring_label) {
            lv_obj_move_foreground(s_ring_label);
        }
        if (s_tap_layer) {
            lv_obj_add_flag(s_tap_layer, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        destroy_ringing_ui();
        if (s_tap_layer) {
            lv_obj_remove_flag(s_tap_layer, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void alarm_long_press_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (chrone_ui_alarm_config_active()) {
        return;
    }
    if (chrone_alarm_get_state() == CHRONE_ALARM_STATE_RINGING) {
        return;
    }

    s_suppress_tap_click = true;
    lv_obj_t *screen = s_screen ? s_screen : lv_scr_act();
    if (screen) {
        chrone_ui_show_alarm_config(screen);
        ESP_LOGI(TAG, "long press %d ms -> alarm config", CLOCK_LONG_PRESS_MS);
    }
}

static void tap_layer_event_cb(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        if (chrone_ui_alarm_config_active()) {
            return;
        }
        if (chrone_alarm_get_state() == CHRONE_ALARM_STATE_RINGING) {
            return;
        }
        s_suppress_tap_click = false;
        if (!s_alarm_long_press_timer) {
            s_alarm_long_press_timer = lv_timer_create(alarm_long_press_timer_cb, CLOCK_LONG_PRESS_MS, nullptr);
            if (s_alarm_long_press_timer) {
                lv_timer_set_repeat_count(s_alarm_long_press_timer, 1);
            }
        } else {
            lv_timer_reset(s_alarm_long_press_timer);
            lv_timer_resume(s_alarm_long_press_timer);
        }
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (s_alarm_long_press_timer) {
            lv_timer_pause(s_alarm_long_press_timer);
        }
        return;
    }

    if (code != LV_EVENT_CLICKED) {
        return;
    }

    if (s_suppress_tap_click) {
        s_suppress_tap_click = false;
        return;
    }

    if (chrone_alarm_get_state() == CHRONE_ALARM_STATE_RINGING) {
        chrone_alarm_dismiss();
        return;
    }

    const uint32_t now = lv_tick_get();
    if (now - s_last_tap_ms < 400) {
        return;
    }
    s_last_tap_ms = now;

    const chrone_ui_mode_t next = (s_mode == CHRONE_UI_MODE_DIGITAL)
                                      ? CHRONE_UI_MODE_ANALOG
                                      : CHRONE_UI_MODE_DIGITAL;
    if (chrone_ui_set_mode(next) == ESP_OK) {
        ESP_LOGI(TAG, "mode -> %s", next == CHRONE_UI_MODE_DIGITAL ? "digital" : "analog");
    }
}

static void create_tap_layer(lv_obj_t *screen)
{
    s_tap_layer = lv_obj_create(screen);
    /* 不覆盖底部 60px，留给 Core2 三区虚拟键（与 AWS Button_Attach 同高） */
    lv_obj_set_size(s_tap_layer, CHRONE_LCD_W, CHRONE_LCD_H - CHRONE_VIRTUAL_BTN_H);
    lv_obj_align(s_tap_layer, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(s_tap_layer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_tap_layer, 0, 0);
    lv_obj_remove_flag(s_tap_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_tap_layer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_tap_layer, tap_layer_event_cb, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(s_tap_layer, tap_layer_event_cb, LV_EVENT_RELEASED, nullptr);
    lv_obj_add_event_cb(s_tap_layer, tap_layer_event_cb, LV_EVENT_PRESS_LOST, nullptr);
    lv_obj_add_event_cb(s_tap_layer, tap_layer_event_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_move_foreground(s_tap_layer);
}

static void release_clock_handles(void)
{
    stop_tick_timer();
    chrone_input_unbind_screen();

    if (s_alarm_long_press_timer) {
        lv_timer_delete(s_alarm_long_press_timer);
        s_alarm_long_press_timer = nullptr;
    }
    s_suppress_tap_click = false;

    /* 响铃层/表盘控件会随 screen 子对象一起删掉，此处只丢弃悬空指针 */
    stop_alarm_icon_blink_timer();
    s_ring_label = nullptr;
    s_analog.detach();

    s_tap_layer = nullptr;
    s_label_date = nullptr;
    s_label_week = nullptr;
    s_alarm_icon_img = nullptr;
    s_prev_alarm_indicator = false;
    s_footer_row = nullptr;
    s_weather_icon_img = nullptr;
    s_label_weather_text = nullptr;
    s_label_temp = nullptr;
    s_label_city = nullptr;
    s_prev_weather_code = -2;
    s_prev_is_day = true;
    s_prov_ui = false;
    s_time_row = nullptr;
    s_lb_hh = nullptr;
    s_lb_c1 = nullptr;
    s_lb_mm = nullptr;
    s_lb_c2 = nullptr;
    s_lb_ss = nullptr;
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

static esp_err_t rebuild_clock_screen(lv_obj_t *screen)
{
    release_clock_handles();
    clear_screen_children(screen);
    reset_tick_cache();

    lv_obj_set_style_bg_color(screen, ui_color_bg(), 0);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    esp_err_t ret;
    if (s_mode == CHRONE_UI_MODE_ANALOG) {
        ret = create_analog_widgets(screen);
    } else {
        ret = create_digital_widgets(screen);
    }
    if (ret != ESP_OK) {
        return ret;
    }

    create_tap_layer(screen);
    chrone_input_bind_screen(screen);
    start_tick_timer();
    chrone_ui_refresh();
    return ESP_OK;
}

extern "C" void chrone_ui_pause_clock(void)
{
    release_clock_handles();
}

extern "C" esp_err_t chrone_ui_resume_clock(lv_obj_t *screen)
{
    if (!screen) {
        return ESP_ERR_INVALID_ARG;
    }
    s_screen = screen;
    return rebuild_clock_screen(screen);
}

extern "C" esp_err_t chrone_ui_init(lv_obj_t *screen)
{
    if (!screen) {
        return ESP_ERR_INVALID_ARG;
    }

    s_screen = screen;
    load_mode_from_nvs();

    esp_err_t ret = rebuild_clock_screen(screen);
    if (ret != ESP_OK) {
        return ret;
    }

    ESP_LOGI(TAG, "clock UI ready (%s, tap=mode, long press %d s=alarm)",
             s_mode == CHRONE_UI_MODE_DIGITAL ? "digital DSEG7" : "analog dial",
             CLOCK_LONG_PRESS_MS / 1000);
    return ESP_OK;
}

extern "C" void chrone_ui_refresh(void)
{
    struct tm tm_local;
    if (chrone_time_get_local_tm(&tm_local) != ESP_OK) {
        return;
    }

    update_header(&tm_local, true);
    update_alarm_indicator();
    update_weather_strip(&tm_local);
    if (s_mode == CHRONE_UI_MODE_DIGITAL) {
        update_time_parts(&tm_local, true);
    } else if (s_analog.is_ready()) {
        s_analog.force_hands_sync(&tm_local);
    }
}

extern "C" chrone_ui_mode_t chrone_ui_get_mode(void)
{
    return s_mode;
}

extern "C" esp_err_t chrone_ui_set_mode(chrone_ui_mode_t mode)
{
    if (mode > CHRONE_UI_MODE_ANALOG) {
        return ESP_ERR_INVALID_ARG;
    }

    s_mode = mode;
    esp_err_t ret = save_mode_to_nvs(mode);
    if (ret != ESP_OK) {
        return ret;
    }

    if (s_screen) {
        ret = rebuild_clock_screen(s_screen);
    }
    return ret;
}
