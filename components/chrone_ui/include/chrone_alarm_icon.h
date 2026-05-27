#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 将 1-bit 闹钟图标渲染为 RGB565（与天气图标约定：1=背景，0=前景） */
void chrone_alarm_icon_render_rgb565(uint16_t *out_rgb565, uint16_t fg_rgb565, uint16_t bg_rgb565);

#ifdef __cplusplus
}
#endif
