#pragma once

#include <stdbool.h>
#include "esp_err.h"
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

#ifdef __cplusplus
}
#endif
