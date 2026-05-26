/**
 * @file weather_code_map.h
 * @brief 心知天气 API 天气代码映射表头文件
 */

#ifndef WEATHER_CODE_MAP_H
#define WEATHER_CODE_MAP_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 根据心知天气 API 的 code 返回对应天气描述（英文，带级别）
 * @param code 天气代码（0-37）
 * @param is_day 白天 06:00–18:00；晴(0–3) 夜间用 Clear 而非 Sunny
 * @param desc_buf 输出缓冲区
 * @param buf_size 缓冲区大小
 */
void weather_code_get_desc(int code, bool is_day, char *desc_buf, size_t buf_size);

/**
 * @brief 检查天气代码是否需要触发震动（本设备无马达，保留实现不调用）
 * @param code 天气代码
 * @return true 雨/雪/恶劣天气，false 否
 */
bool weather_code_should_vibrate(int code);

#ifdef __cplusplus
}
#endif

#endif // WEATHER_CODE_MAP_H
