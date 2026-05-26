#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化震动马达
 * 
 * @return int 0成功，-1失败
 */
int vibration_init(void);

/**
 * @brief 启动震动
 * 
 * @param strength 震动强度（0-255，建议80-200）
 * @param duration_ms 震动持续时间（毫秒）
 * @return int 0成功，-1失败
 */
int vibration_start(uint8_t strength, uint16_t duration_ms);

/**
 * @brief 停止震动
 */
void vibration_stop(void);

/**
 * @brief 反初始化震动马达（释放资源）
 */
void vibration_deinit(void);

#ifdef __cplusplus
}
#endif

