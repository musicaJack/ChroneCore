#include "chrone_input.h"

#include "esp_lcd_touch.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "chrone_input";

static esp_lcd_touch_handle_t s_touch;
static bool s_left;
static bool s_right;
static bool s_mid;

static bool s_lv_left;
static bool s_lv_right;
static bool s_lv_mid;
static lv_obj_t *s_zone_left;
static lv_obj_t *s_zone_mid;
static lv_obj_t *s_zone_right;

static bool point_in_btn(uint16_t x, uint16_t y, uint16_t bx, uint16_t by, uint16_t bw, uint16_t bh)
{
    return !((x < bx) || (x > (bx + bw)) || (y < by) || (y > (by + bh)));
}

static int classify_bottom_key(uint16_t x, uint16_t y)
{
    if (y >= CHRONE_BTN_AWS_Y0 && y <= (CHRONE_BTN_AWS_Y0 + CHRONE_BTN_H)) {
        if (point_in_btn(x, y, 0, CHRONE_BTN_AWS_Y0, CHRONE_BTN_W, CHRONE_BTN_H)) {
            return 0;
        }
        if (point_in_btn(x, y, CHRONE_BTN_W, CHRONE_BTN_AWS_Y0, CHRONE_BTN_W, CHRONE_BTN_H)) {
            return 1;
        }
        if (point_in_btn(x, y, 212, CHRONE_BTN_AWS_Y0, CHRONE_BTN_W, CHRONE_BTN_H)) {
            return 2;
        }
        return -1;
    }

    if (y >= CHRONE_BTN_LAND_Y0 && y < CHRONE_LCD_H) {
        if (x <= CHRONE_BTN_W) {
            return 0;
        }
        if (x <= (CHRONE_BTN_W * 2)) {
            return 1;
        }
        return 2;
    }

    return -1;
}

static void zone_event_cb(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_RELEASED) {
        return;
    }
    const bool down = (code == LV_EVENT_PRESSED);
    const intptr_t id = (intptr_t)lv_event_get_user_data(e);
    if (id == 0) {
        s_lv_left = down;
    } else if (id == 1) {
        s_lv_mid = down;
    } else {
        s_lv_right = down;
    }
}

static lv_obj_t *create_bottom_zone(lv_obj_t *parent, lv_align_t align, int x_ofs, int w, intptr_t id)
{
    lv_obj_t *z = lv_obj_create(parent);
    lv_obj_set_size(z, w, CHRONE_BTN_H);
    lv_obj_align(z, align, x_ofs, 0);
    lv_obj_set_style_bg_opa(z, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(z, 0, 0);
    lv_obj_remove_flag(z, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(z, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(z, zone_event_cb, LV_EVENT_PRESSED, (void *)id);
    lv_obj_add_event_cb(z, zone_event_cb, LV_EVENT_RELEASED, (void *)id);
    return z;
}

static void read_touch(void)
{
    bool raw_left = false;
    bool raw_right = false;
    bool raw_mid = false;

    if (!s_touch) {
        s_left = s_lv_left;
        s_right = s_lv_right;
        s_mid = s_lv_mid;
        return;
    }

    esp_lcd_touch_read_data(s_touch);

    esp_lcd_touch_point_data_t pts[5];
    uint8_t count = 0;
    if (!esp_lcd_touch_get_data(s_touch, pts, &count, 5) || count == 0) {
        s_left = s_lv_left;
        s_right = s_lv_right;
        s_mid = s_lv_mid;
        return;
    }

    for (uint8_t i = 0; i < count; ++i) {
        const int zone = classify_bottom_key(pts[i].x, pts[i].y);
        if (zone == 0) {
            raw_left = true;
        } else if (zone == 1) {
            raw_mid = true;
        } else if (zone == 2) {
            raw_right = true;
        }
    }

    s_left = raw_left || s_lv_left;
    s_right = raw_right || s_lv_right;
    s_mid = raw_mid || s_lv_mid;
}

esp_err_t chrone_input_init(esp_lcd_touch_handle_t touch)
{
    s_touch = touch;
    s_lv_left = s_lv_right = s_lv_mid = false;
    ESP_LOGI(TAG, "bottom virtual keys ready");
    return touch ? ESP_OK : ESP_ERR_INVALID_STATE;
}

void chrone_input_bind_screen(lv_obj_t *screen)
{
    if (!screen) {
        return;
    }

    if (s_zone_left) {
        lv_obj_delete(s_zone_left);
        s_zone_left = NULL;
        s_zone_mid = NULL;
        s_zone_right = NULL;
    }

    s_zone_left = create_bottom_zone(screen, LV_ALIGN_BOTTOM_LEFT, 0, CHRONE_BTN_W, 0);
    s_zone_mid = create_bottom_zone(screen, LV_ALIGN_BOTTOM_MID, 0, CHRONE_BTN_W, 1);
    s_zone_right = create_bottom_zone(screen, LV_ALIGN_BOTTOM_RIGHT, 0, CHRONE_BTN_W, 2);

    lv_obj_move_foreground(s_zone_left);
    lv_obj_move_foreground(s_zone_mid);
    lv_obj_move_foreground(s_zone_right);
}

void chrone_input_unbind_screen(void)
{
    if (s_zone_left) {
        lv_obj_delete(s_zone_left);
        s_zone_left = NULL;
        s_zone_mid = NULL;
        s_zone_right = NULL;
    }
    s_lv_left = s_lv_right = s_lv_mid = false;
}

void chrone_input_poll(void)
{
    read_touch();
}

bool chrone_input_left_down(void)
{
    return s_left;
}

bool chrone_input_right_down(void)
{
    return s_right;
}

bool chrone_input_middle_down(void)
{
    return s_mid;
}
