#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 将 1-bit 闹钟图标渲染为 RGB565（与天气图标约定：1=背景，0=前景）
 *  输出缓冲须 ≥ CHRONE_ALARM_ICON_CANVAS_W × CHRONE_ALARM_ICON_CANVAS_H
 *  badge_count: 图标右上外侧 4×6 小数字（1..9），0 表示不绘制 */
void chrone_alarm_icon_render_rgb565(uint16_t *out_rgb565, uint16_t fg_rgb565, uint16_t bg_rgb565,
                                    uint8_t badge_count);

#ifdef __cplusplus
}
#endif
