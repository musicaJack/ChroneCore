/**
 * @file vibration.c
 * @brief 震动马达控制模块实现
 * @note M5Stack Core2的震动马达通过AXP192的LDO3控制，使用BSP API
 */

#include "vibration/vibration.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/m5stack_core_2.h"  // 使用BSP API控制震动马达

static const char *TAG = "VIBRATION";

static bool s_vibration_initialized = false;
static TaskHandle_t s_vibration_task_handle = NULL;

/**
 * @brief 震动控制任务（用于自动停止震动）
 */
static void vibration_task(void *pvParameters) {
    uint16_t duration_ms = (uint16_t)(uintptr_t)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    
    // 停止震动：禁用震动功能（震动时长已到，正常停止）
    esp_err_t ret = bsp_feature_enable(BSP_FEATURE_VIBRATION, false);
    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "震动已完成（时长已到，正常停止）");  // 改为DEBUG级别，避免误导
    } else {
        ESP_LOGW(TAG, "停止震动失败: %s", esp_err_to_name(ret));
    }
    
    s_vibration_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t vibration_init(void) {
    if (s_vibration_initialized) {
        ESP_LOGW(TAG, "震动马达已初始化");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "初始化震动马达（使用BSP API）...");
    
    // 使用BSP API初始化震动马达（这会设置LDO3电压并准备好控制）
    // 注意：这里只配置，不启用震动
    esp_err_t ret = bsp_feature_enable(BSP_FEATURE_VIBRATION, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "初始化震动功能失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 短暂延迟，确保BSP配置完成
    vTaskDelay(pdMS_TO_TICKS(10));

    s_vibration_initialized = true;
    ESP_LOGI(TAG, "震动马达初始化成功（BSP已配置）");
    return ESP_OK;
}

esp_err_t vibration_start(uint8_t strength, uint16_t duration_ms) {
    if (!s_vibration_initialized) {
        ESP_LOGE(TAG, "震动马达未初始化，请先调用 vibration_init()");
        return ESP_ERR_INVALID_STATE;
    }

    // 如果已有震动任务在运行，先停止它
    if (s_vibration_task_handle != NULL) {
        vTaskDelete(s_vibration_task_handle);
        s_vibration_task_handle = NULL;
        // 停止震动
        bsp_feature_enable(BSP_FEATURE_VIBRATION, false);
    }

    if (strength == 0) {
        // 强度为0，直接停止
        bsp_feature_enable(BSP_FEATURE_VIBRATION, false);
        return ESP_OK;
    }

    // 启用震动马达（使用BSP API）
    esp_err_t ret = bsp_feature_enable(BSP_FEATURE_VIBRATION, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "启用震动失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 如果需要自动停止，创建任务
    if (duration_ms > 0) {
        BaseType_t task_ret = xTaskCreate(
            vibration_task,
            "vibration_task",
            2048,  // 堆栈大小
            (void*)(uintptr_t)duration_ms,  // 传递时长参数
            5,     // 任务优先级
            &s_vibration_task_handle
        );
        
        if (task_ret != pdPASS) {
            ESP_LOGE(TAG, "创建震动任务失败");
            // 停止震动
            bsp_feature_enable(BSP_FEATURE_VIBRATION, false);
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "🔔 震动启动：强度=%d（固定），时长=%dms（将自动停止）", strength, duration_ms);
    return ESP_OK;
}

esp_err_t vibration_stop(void) {
    if (!s_vibration_initialized) {
        return ESP_OK;  // 未初始化也返回成功
    }

    // 删除自动停止任务（如果存在）
    if (s_vibration_task_handle != NULL) {
        vTaskDelete(s_vibration_task_handle);
        s_vibration_task_handle = NULL;
    }

    // 停止震动：禁用震动功能（使用BSP API）
    esp_err_t ret = bsp_feature_enable(BSP_FEATURE_VIBRATION, false);
    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "震动已停止");  // 改为DEBUG级别
    }
    return ret;
}

esp_err_t vibration_trigger(void) {
    ESP_LOGI(TAG, "🔔 触发震动（默认强度=%d，时长=%dms）", VIBRATION_DEFAULT_STRENGTH, VIBRATION_DEFAULT_DURATION_MS);
    return vibration_start(VIBRATION_DEFAULT_STRENGTH, VIBRATION_DEFAULT_DURATION_MS);
}

esp_err_t vibration_deinit(void) {
    if (!s_vibration_initialized) {
        return ESP_OK;
    }

    vibration_stop();
    
    // 注意：我们不删除I2C总线，因为它由AXP192管理
    
    s_vibration_initialized = false;
    ESP_LOGI(TAG, "震动马达资源已清理");
    return ESP_OK;
}
