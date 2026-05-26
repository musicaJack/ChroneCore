#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 启动应用 UI（占位主屏，后续接时钟/闹钟） */
esp_err_t chrone_app_start(void);

#ifdef __cplusplus
}
#endif
