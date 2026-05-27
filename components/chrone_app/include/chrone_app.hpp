#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 启动应用 UI（时钟 + WiFi 后台任务） */
esp_err_t chrone_app_start(void);

/** 50ms 虚拟键 / 摇停 / 响铃 UI 轮询 */
esp_err_t chrone_app_poll_init(void);

#ifdef __cplusplus
}
#endif
