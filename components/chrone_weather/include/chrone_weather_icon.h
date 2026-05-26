#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHRONE_WEATHER_ICON_SZ 24

/**
 * @brief 将 48×48 1-bit 心知图标下采样为 24×24 RGB565（透明底用 bg 色）
 * @param code 心知天气码，<0 则清空为 bg
 * @param is_day 6:00–18:00 为 true；传 false 时 map 内可自动判断
 * @param out_rgb565 长度至少 24*24
 * @param fg_rgb565 前景（图标线条）
 * @param bg_rgb565 背景（与屏背景一致）
 */
void chrone_weather_icon_render_rgb565(int code, bool is_day, uint16_t *out_rgb565,
                                       uint16_t fg_rgb565, uint16_t bg_rgb565);

#ifdef __cplusplus
}
#endif
