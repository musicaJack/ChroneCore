#include "chrone_hal.h"

#include "axp192.h"
#include "bsp/display.h"
#include "bsp/esp-bsp.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "chrone_hal";

static axp192_handle_t s_axp192;
static lv_display_t *s_display;

static esp_err_t init_axp192_power(void)
{
    axp192_config_t cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_pin = GPIO_NUM_21,
        .scl_pin = GPIO_NUM_22,
        .i2c_freq = 400000,
        .i2c_addr = 0x34,
    };

    esp_err_t ret = axp192_create(&cfg, &s_axp192);
    if (ret != ESP_OK || s_axp192 == NULL) {
        ESP_LOGE(TAG, "axp192_create failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ret = axp192_init(s_axp192);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "axp192_init failed: %s", esp_err_to_name(ret));
        axp192_destroy(s_axp192);
        s_axp192 = NULL;
        return ESP_FAIL;
    }

    ret = axp192_set_dcdc1_voltage(s_axp192, 3300);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "dcdc1 voltage: %s", esp_err_to_name(ret));
    }
    ret = axp192_enable_dcdc1(s_axp192);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "dcdc1 enable: %s", esp_err_to_name(ret));
    }

    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI(TAG, "AXP192 ready");
    return ESP_OK;
}

static esp_err_t init_display(void)
{
    /* Do not use bsp_display_start_with_config(): it always calls bsp_touch_new(),
     * which sets scl_speed_hz in FT5x06 I2C config — invalid with IDF 5.5 legacy
     * driver (ESP_ERR_INVALID_ARG, reboot loop). Touch can be added in phase 2. */
    const uint32_t buffer_size = BSP_LCD_H_RES * 40;
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = buffer_size,
        .double_buffer = true,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
        },
    };
    cfg.lvgl_port_cfg.task_affinity = -1;

    esp_err_t ret = lvgl_port_init(&cfg.lvgl_port_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "lvgl_port_init: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = bsp_display_brightness_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "bsp_display_brightness_init: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_handle_t panel_handle = NULL;
    const bsp_display_config_t bsp_disp_cfg = {
        .max_transfer_sz = buffer_size * sizeof(uint16_t),
    };
    ret = bsp_display_new(&bsp_disp_cfg, &panel_handle, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "bsp_display_new: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_lcd_panel_disp_on_off(panel_handle, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "panel on: %s", esp_err_to_name(ret));
        return ret;
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = buffer_size,
        .double_buffer = cfg.double_buffer,
        .hres = BSP_LCD_H_RES,
        .vres = BSP_LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = cfg.flags.buff_dma,
            .buff_spiram = cfg.flags.buff_spiram,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = (BSP_LCD_BIGENDIAN ? true : false),
#endif
        },
    };

    s_display = lvgl_port_add_disp(&disp_cfg);
    if (s_display == NULL) {
        ESP_LOGE(TAG, "lvgl_port_add_disp failed");
        return ESP_FAIL;
    }

    ret = bsp_display_brightness_set(60);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "brightness: %s", esp_err_to_name(ret));
    }

    ret = chrone_hal_touch_init(s_display);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "touch init skipped (%s); tap-to-switch unavailable", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "display ready (320x240 LVGL)");
    return ESP_OK;
}

esp_err_t chrone_hal_init(void)
{
    if (init_axp192_power() != ESP_OK) {
        return ESP_FAIL;
    }
    if (init_display() != ESP_OK) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

bool chrone_hal_display_ready(void)
{
    return s_display != NULL;
}
