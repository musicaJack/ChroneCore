#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_lcd_touch.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Core2 底部三区虚拟键（与 Core2-for-AWS core2forAWS.c 一致）：
 *   Button_Attach(0,   240, 106, 60)  左
 *   Button_Attach(106, 240, 106, 60)  中
 *   Button_Attach(212, 240, 106, 60)  右
 *
 * ChroneCore 横屏 320×240 时，LVGL 坐标底栏约为 y∈[180,240)；
 * 原始触摸有时与 AWS 一致报告 y≥240，故两套区域均参与判定。
 */
#define CHRONE_LCD_W           320
#define CHRONE_LCD_H           240
#define CHRONE_BTN_W           106
#define CHRONE_BTN_H           60
#define CHRONE_BTN_LAND_Y0     (CHRONE_LCD_H - CHRONE_BTN_H)
#define CHRONE_BTN_AWS_Y0      240

esp_err_t chrone_input_init(esp_lcd_touch_handle_t touch);

/** 在时钟屏底部创建透明 LVGL 热区（与 AWS 三区同尺寸，双路径检测） */
void chrone_input_bind_screen(lv_obj_t *screen);
void chrone_input_unbind_screen(void);

/** 轮询触摸（建议 50ms，调用方宜持有 LVGL 锁） */
void chrone_input_poll(void);

bool chrone_input_left_down(void);
bool chrone_input_right_down(void);
bool chrone_input_middle_down(void);

#ifdef __cplusplus
}
#endif
