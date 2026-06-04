#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void chrone_display_idle_init(void);

/** 任意用户触摸（时钟或 Settings 内） */
void chrone_display_idle_on_touch(void);

/** 恢复用户亮度（闹钟响铃、进 Settings 前调用） */
void chrone_display_idle_wake(void);

/** 主时钟前台时周期调用（如 50ms/1s） */
void chrone_display_idle_tick(bool clock_screen_active);

bool chrone_display_idle_is_off(void);

#ifdef __cplusplus
}
#endif
