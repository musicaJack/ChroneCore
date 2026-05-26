#pragma once

#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 从 BM8563 读时间并写入系统时钟（settimeofday） */
esp_err_t chrone_time_init(void);

/** 读取当前本地时间（依赖已设置的系统时区，默认 CST-8 可在后续 Kconfig） */
esp_err_t chrone_time_get_local_tm(struct tm *out);

/** 写系统时间并回写 BM8563 */
esp_err_t chrone_time_set_local_tm(const struct tm *in);

/** 系统时间是否已从 RTC 成功同步 */
bool chrone_time_rtc_synced(void);

/** tm_wday 0=Sunday → 中文星期 */
const char *chrone_time_weekday_zh(int tm_wday);
const char *chrone_time_weekday_en(int tm_wday);

/**
 * @brief 配置 SNTP（WiFi 连接后调用 chrone_time_sync_sntp_start）
 * @note 阶段 3 联网后启用；当前可安全调用 start，无 WiFi 时仅重试
 */
esp_err_t chrone_time_sync_sntp_start(void);
bool chrone_time_sntp_synced(void);

#ifdef __cplusplus
}
#endif
