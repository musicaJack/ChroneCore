/**
 * @file weather_icon_map.h
 * @brief 心知天气代码到天气图标的映射
 */

#ifndef WEATHER_ICON_MAP_H
#define WEATHER_ICON_MAP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 根据心知天气代码获取对应的图标位图
 * 
 * @param code 心知天气代码 (0-37)
 * @param is_day 是否为白天 (true=白天, false=夜晚)
 * @return const uint8_t* 图标位图数据指针，48x48像素
 */
const uint8_t* weather_icon_get_bitmap(int code, bool is_day);

#ifdef __cplusplus
}
#endif

#endif // WEATHER_ICON_MAP_H
