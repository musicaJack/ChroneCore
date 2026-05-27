#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CHRONE_ALARM_MAX 4

typedef enum {
    CHRONE_ALARM_REPEAT_DAILY = 0,
    CHRONE_ALARM_REPEAT_WORKDAY = 1,
    CHRONE_ALARM_REPEAT_ONCE = 2,
} chrone_alarm_repeat_t;

/** Once：距下一次触发的最短提前量（分钟） */
#define CHRONE_ALARM_ONCE_MIN_LEAD_MIN 2

typedef struct {
    bool enabled;
    uint8_t hour;
    uint8_t minute;
    uint8_t repeat;
    uint8_t flags; /* reserved, always 0 */
} chrone_alarm_t;

typedef enum {
    CHRONE_ALARM_STATE_IDLE = 0,
    CHRONE_ALARM_STATE_RINGING,
} chrone_alarm_state_t;

esp_err_t chrone_alarm_load(void);
esp_err_t chrone_alarm_save(void);
esp_err_t chrone_alarm_get(uint8_t index, chrone_alarm_t *out);
esp_err_t chrone_alarm_set(uint8_t index, const chrone_alarm_t *alarm);

bool chrone_alarm_scheduling_enabled(void);
/** 已开启（enabled）的闹钟条数，0..CHRONE_ALARM_MAX */
uint8_t chrone_alarm_enabled_count(void);
/** Once 且将保存为 ON 时：下次触发须 ≥ CHRONE_ALARM_ONCE_MIN_LEAD_MIN 分钟后 */
bool chrone_alarm_once_time_ok(uint8_t hour, uint8_t minute);
chrone_alarm_state_t chrone_alarm_get_state(void);
int chrone_alarm_ringing_index(void);

/** 每秒调用（本地 struct tm） */
void chrone_alarm_check_tick(const struct tm *now);

/** 50ms 轮询：60s 超时等 */
void chrone_alarm_poll(void);

void chrone_alarm_dismiss(void);

#ifdef __cplusplus
}
#endif
