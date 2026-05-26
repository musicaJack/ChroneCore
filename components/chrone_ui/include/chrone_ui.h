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

#ifdef __cplusplus
}
#endif
