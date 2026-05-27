#pragma once

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CHRONE_UI_MODE_DIGITAL = 0,
    CHRONE_UI_MODE_ANALOG  = 1,
} chrone_ui_mode_t;

/** Build clock screen on active LVGL screen; call under display lock. */
esp_err_t chrone_ui_init(lv_obj_t *screen);

/** Refresh header + time (call ~1 Hz). */
void chrone_ui_refresh(void);

chrone_ui_mode_t chrone_ui_get_mode(void);
esp_err_t chrone_ui_set_mode(chrone_ui_mode_t mode);

/** WiFi/天气状态（由 1 Hz 定时器调用） */
void chrone_ui_poll_services(void);

/** 停止时钟 1 Hz 定时器（进闹钟配置前调用） */
void chrone_ui_pause_clock(void);

/** 从闹钟界面返回时重建主钟屏（勿再调用 chrone_ui_init，避免重复释放 LVGL 对象） */
esp_err_t chrone_ui_resume_clock(lv_obj_t *screen);

/** 闹钟配置界面（会清空当前屏子对象） */
void chrone_ui_show_alarm_config(lv_obj_t *screen);
void chrone_ui_show_clock(void);
bool chrone_ui_alarm_config_active(void);

/** 响铃全屏层（触摸停止） */
void chrone_ui_update_ringing(void);

#ifdef __cplusplus
}
#endif
