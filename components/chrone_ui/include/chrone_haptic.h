#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 进入闹钟界面前调用，避免首次拨动才初始化震动 */
void chrone_haptic_prepare(void);

/** 滚轮每跨一格（短脉冲，快速连续拨动不会互相取消） */
void chrone_haptic_detent(void);

/** Save / 确认（稍长一拍） */
void chrone_haptic_confirm(void);

/** 震动档位预览（设置页，与导航触觉独立） */
void chrone_haptic_vibe_preview(uint8_t level);

#ifdef __cplusplus
}
#endif
