#include "chrone_hal.h"

#include "bsp/m5stack_core_2.h"
#include "bsp/touch.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

static const char *TAG = "chrone_hal";
static esp_lcd_touch_handle_t s_touch;
static lv_indev_t *s_lv_touch_indev;

static esp_err_t init_touch_indev(lv_display_t *disp)
{
    if (!disp) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = bsp_feature_enable(BSP_FEATURE_TOUCH, true);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "touch power: %s", esp_err_to_name(ret));
        return ret;
    }

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_H_RES,
        .y_max = BSP_LCD_V_RES,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = BSP_LCD_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    esp_lcd_panel_io_handle_t tp_io = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    /* Legacy I2C driver (bsp_i2c_init): scl_speed_hz must be 0 */
    tp_io_cfg.scl_speed_hz = 0;

    ret = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)BSP_I2C_NUM, &tp_io_cfg, &tp_io);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "touch panel_io: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_lcd_touch_handle_t tp = NULL;
    ret = esp_lcd_touch_new_i2c_ft5x06(tp_io, &tp_cfg, &tp);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "touch ft5x06: %s", esp_err_to_name(ret));
        return ret;
    }
    s_touch = tp;

    const lvgl_port_touch_cfg_t lv_touch = {
        .disp = disp,
        .handle = tp,
    };
    s_lv_touch_indev = lvgl_port_add_touch(&lv_touch);
    if (s_lv_touch_indev == NULL) {
        ESP_LOGW(TAG, "lvgl_port_add_touch failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "FT5x06 touch -> LVGL indev");
    return ESP_OK;
}

esp_err_t chrone_hal_touch_init(lv_display_t *disp)
{
    esp_err_t ret = bsp_i2c_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "bsp_i2c_init: %s", esp_err_to_name(ret));
        return ret;
    }
    return init_touch_indev(disp);
}

esp_lcd_touch_handle_t chrone_hal_get_touch(void)
{
    return s_touch;
}

void chrone_hal_touch_lvgl_enable(bool enable)
{
    if (s_lv_touch_indev) {
        lv_indev_enable(s_lv_touch_indev, enable);
    }
}

bool chrone_hal_touch_any_pressed(void)
{
    if (!s_touch) {
        return false;
    }

    if (esp_lcd_touch_read_data(s_touch) != ESP_OK) {
        return false;
    }

    esp_lcd_touch_point_data_t pt[1];
    uint8_t cnt = 0;
    if (esp_lcd_touch_get_data(s_touch, pt, &cnt, 1) != ESP_OK) {
        return false;
    }
    return cnt > 0;
}
