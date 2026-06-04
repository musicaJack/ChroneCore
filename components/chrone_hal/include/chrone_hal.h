#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_lcd_touch.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 板级初始化：AXP192 供电、BSP 显示 + LVGL、背光。
 * @note 须在 app_main 中于 NVS 初始化之后调用。
 */
esp_err_t chrone_hal_init(void);

/** 显示与 LVGL 是否已初始化 */
bool chrone_hal_display_ready(void);

/** BSP I2C + FT5x06 触摸注册到 LVGL（须在 lvgl_port_add_disp 之后调用） */
esp_err_t chrone_hal_touch_init(lv_display_t *disp);

/** 触摸句柄（touch_init 成功后有效，供 chrone_input 轮询） */
esp_lcd_touch_handle_t chrone_hal_get_touch(void);

/**
 * @brief 设置背光亮度（1–100）；内部确保 AXP192 DCDC3 已开启。
 * @note M5Stack Core2 上 bsp_display_brightness_set(0) 不会关断背光，仅调压。
 */
esp_err_t chrone_hal_set_brightness(uint8_t percent);

/** 背光调至最低（不关 DCDC3，避免与 BSP/触摸争用 I2C） */
esp_err_t chrone_hal_backlight_off(void);

/** 关屏时暂停 LVGL 触摸轮询，避免 I2C 超时触发 abort */
void chrone_hal_touch_lvgl_enable(bool enable);

/** 直接读 FT5x06（关屏时用于唤醒，不经过 LVGL） */
bool chrone_hal_touch_any_pressed(void);

#ifdef __cplusplus
}
#endif
