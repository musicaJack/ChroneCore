/**
 * @file vibration.h
 * @brief 震动马达控制模块
 * @details 用于M5Stack Core2震动马达控制（通过AXP192 LDO3，使用BSP API）
 * 
 * @author Auto Generated
 * @date 2025-11-02
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// 默认震动配置
#define VIBRATION_DEFAULT_STRENGTH  150        // 默认震动强度（仅用于API兼容性，实际固定强度）
#define VIBRATION_DEFAULT_DURATION_MS 100      // 默认震动时长（毫秒）

/**
 * @brief 初始化震动马达（使用BSP API配置AXP192 LDO3）
 * 
 * @return esp_err_t ESP_OK成功，其他值表示失败
 */
esp_err_t vibration_init(void);

/**
 * @brief 启动震动
 * 
 * @param strength 震动强度（仅用于API兼容性，实际固定强度，由BSP控制）
 * @param duration_ms 震动时长（毫秒），0表示持续震动直到手动停止
 * @return esp_err_t ESP_OK成功，其他值表示失败
 */
esp_err_t vibration_start(uint8_t strength, uint16_t duration_ms);

/**
 * @brief 停止震动
 * 
 * @return esp_err_t ESP_OK成功，其他值表示失败
 */
esp_err_t vibration_stop(void);

/**
 * @brief 触发一次短震动（使用默认强度和时长）
 * 
 * @return esp_err_t ESP_OK成功，其他值表示失败
 */
esp_err_t vibration_trigger(void);

/**
 * @brief 清理震动马达资源
 * 
 * @return esp_err_t ESP_OK成功，其他值表示失败
 */
esp_err_t vibration_deinit(void);

#ifdef __cplusplus
}
#endif

