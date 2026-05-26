/**
 * @file axp192.c
 * @brief AXP192 power management chip driver implementation
 * @description Implementation of AXP192 driver for M5Stack modules
 */

#include "axp192.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"  // 只使用旧版I2C驱动，避免新旧驱动冲突
#include <string.h>  // memset

static const char* TAG = "AXP192";

// AXP192句柄结构
struct axp192_handle {
    axp192_config_t config;
    bool is_initialized;
    // 新版I2C Master API支持
    i2c_master_dev_handle_t device_handle;  // 新版API的设备句柄
    bool use_v2_api;  // 是否使用新版API
};

/**
 * @brief 读取AXP192寄存器（支持旧版和新版API）
 */
static esp_err_t axp192_read_register(axp192_handle_t handle, uint8_t reg, uint8_t* data) {
    if (!handle || !data) {
        return ESP_ERR_INVALID_ARG;
    }

    // 注意：已移除新版I2C Master API支持，只使用旧版API以避免驱动冲突
    // 使用旧版I2C API
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (handle->config.i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (handle->config.i2c_addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(handle->config.i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return ret;
}

/**
 * @brief 写入AXP192寄存器（支持旧版和新版API）
 */
static esp_err_t axp192_write_register(axp192_handle_t handle, uint8_t reg, uint8_t data) {
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    // 注意：已移除新版I2C Master API支持，只使用旧版API以避免驱动冲突
    // 使用旧版I2C API
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (handle->config.i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(handle->config.i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return ret;
}

/**
 * @brief 修改AXP192寄存器位
 */
static esp_err_t axp192_modify_register(axp192_handle_t handle, uint8_t reg, uint8_t mask, uint8_t value) {
    uint8_t current_value;
    esp_err_t ret = axp192_read_register(handle, reg, &current_value);
    if (ret != ESP_OK) {
        return ret;
    }

    current_value = (current_value & ~mask) | (value & mask);
    return axp192_write_register(handle, reg, current_value);
}

esp_err_t axp192_create(const axp192_config_t* config, axp192_handle_t* handle_out) {
    if (!config || !handle_out) {
        return ESP_ERR_INVALID_ARG;
    }

    // 分配句柄内存
    axp192_handle_t handle = (axp192_handle_t)malloc(sizeof(struct axp192_handle));
    if (!handle) {
        ESP_LOGE(TAG, "Failed to allocate memory for AXP192 handle");
        return ESP_ERR_NO_MEM;
    }

    // 复制配置
    handle->config = *config;
    handle->is_initialized = false;
    handle->device_handle = NULL;
    handle->use_v2_api = false;  // 旧版API

    *handle_out = handle;
    ESP_LOGI(TAG, "AXP192 handle created successfully (legacy API)");
    return ESP_OK;
}

esp_err_t axp192_destroy(axp192_handle_t handle) {
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    free(handle);
    ESP_LOGI(TAG, "AXP192 handle destroyed");
    return ESP_OK;
}

esp_err_t axp192_init(axp192_handle_t handle) {
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->is_initialized) {
        ESP_LOGW(TAG, "AXP192 already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing AXP192...");

    // 配置I2C（使用旧版API）
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = handle->config.sda_pin,
        .scl_io_num = handle->config.scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = handle->config.i2c_freq,
    };

    esp_err_t ret = i2c_param_config(handle->config.i2c_port, &i2c_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure I2C: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(handle->config.i2c_port, i2c_config.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(ret));
        return ret;
    }

    // 测试AXP192通信
    uint8_t power_status;
    ret = axp192_read_register(handle, AXP192_REG_POWER_STATUS, &power_status);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to communicate with AXP192: %s", esp_err_to_name(ret));
        i2c_driver_delete(handle->config.i2c_port);
        return ret;
    }

    ESP_LOGI(TAG, "AXP192 communication test successful, power status: 0x%02X", power_status);

    // 读取并显示当前EXTEN_DCDC2寄存器状态
    uint8_t exten_dcdc2_status;
    ret = axp192_read_register(handle, AXP192_REG_EXTEN_DCDC2, &exten_dcdc2_status);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Current EXTEN_DCDC2 register: 0x%02X", exten_dcdc2_status);
    }

    // 初始化LCD电源和背光
    // 使能DCDC3 (LCD背光电源) - 根据搜索结果，DCDC3启用位可能不同
    ret = axp192_modify_register(handle, AXP192_REG_EXTEN_DCDC2, AXP192_DCDC3_ENABLE, AXP192_DCDC3_ENABLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable DCDC3: %s", esp_err_to_name(ret));
        i2c_driver_delete(handle->config.i2c_port);
        return ret;
    }

    // 设置DCDC3电压 - 根据搜索结果，可能需要不同的电压值
    // 尝试设置为2.8V (0x14) 而不是3.3V
    ret = axp192_write_register(handle, AXP192_REG_DCDC3_VOLTAGE, 0x14);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set DCDC3 voltage: %s", esp_err_to_name(ret));
        i2c_driver_delete(handle->config.i2c_port);
        return ret;
    }
    ESP_LOGI(TAG, "DCDC3 (LCD backlight) enabled and set to 2.8V");

    // 根据原理图，LDO2输出PERI_VDD，为LCD模块的VDD供电
    // 必须启用LDO2并设置其电压
    ret = axp192_modify_register(handle, AXP192_REG_EXTEN_DCDC2, AXP192_LDO2_ENABLE, AXP192_LDO2_ENABLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable LDO2: %s", esp_err_to_name(ret));
        i2c_driver_delete(handle->config.i2c_port);
        return ret;
    }

    // 设置LDO2电压为3.3V (0x1F = 3.3V)
    ret = axp192_write_register(handle, AXP192_REG_LDO2_VOLTAGE, 0x1F);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LDO2 voltage: %s", esp_err_to_name(ret));
        i2c_driver_delete(handle->config.i2c_port);
        return ret;
    }
    ESP_LOGI(TAG, "LDO2 (PERI_VDD) enabled and set to 3.3V for LCD VDD");

    // 读取并显示配置后的EXTEN_DCDC2寄存器状态
    ret = axp192_read_register(handle, AXP192_REG_EXTEN_DCDC2, &exten_dcdc2_status);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Final EXTEN_DCDC2 register: 0x%02X", exten_dcdc2_status);
    }

    // 配置GPIO4为输出模式 (LCD复位) - 根据搜索结果，GPIO配置方式可能不同
    // 先读取当前GPIO4配置
    uint8_t gpio4_current;
    ret = axp192_read_register(handle, AXP192_REG_GPIO4_CTRL, &gpio4_current);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Current GPIO4 register: 0x%02X", gpio4_current);
    }
    
    // 设置GPIO4为输出模式并拉高
    ret = axp192_write_register(handle, AXP192_REG_GPIO4_CTRL, AXP192_GPIO_OUTPUT_HIGH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO4: %s", esp_err_to_name(ret));
        i2c_driver_delete(handle->config.i2c_port);
        return ret;
    }
    ESP_LOGI(TAG, "GPIO4 configured for LCD reset control");

    // 最终状态检查
    ESP_LOGI(TAG, "=== AXP192 Final Status Check ===");
    
    // 检查EXTEN_DCDC2寄存器
    ret = axp192_read_register(handle, AXP192_REG_EXTEN_DCDC2, &exten_dcdc2_status);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "EXTEN_DCDC2: 0x%02X (DCDC1:%d DCDC3:%d LDO2:%d LDO3:%d)", 
                 exten_dcdc2_status,
                 (exten_dcdc2_status & AXP192_DCDC1_ENABLE) ? 1 : 0,
                 (exten_dcdc2_status & AXP192_DCDC3_ENABLE) ? 1 : 0,
                 (exten_dcdc2_status & AXP192_LDO2_ENABLE) ? 1 : 0,
                 (exten_dcdc2_status & AXP192_LDO3_ENABLE) ? 1 : 0);
    }
    
    // 检查DCDC3电压
    uint8_t dcdc3_voltage;
    ret = axp192_read_register(handle, AXP192_REG_DCDC3_VOLTAGE, &dcdc3_voltage);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "DCDC3 voltage register: 0x%02X", dcdc3_voltage);
    }
    
    // 检查LDO2电压
    uint8_t ldo2_voltage;
    ret = axp192_read_register(handle, AXP192_REG_LDO2_VOLTAGE, &ldo2_voltage);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "LDO2 voltage register: 0x%02X", ldo2_voltage);
    }
    
    // 检查GPIO4状态
    ret = axp192_read_register(handle, AXP192_REG_GPIO4_CTRL, &gpio4_current);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "GPIO4 register: 0x%02X", gpio4_current);
    }
    
    ESP_LOGI(TAG, "=== AXP192 Status Check Complete ===");

    handle->is_initialized = true;
    ESP_LOGI(TAG, "AXP192 initialized successfully");
    return ESP_OK;
}

esp_err_t axp192_set_gpio(axp192_handle_t handle, uint8_t gpio_num, bool level) {
    if (!handle || !handle->is_initialized || gpio_num > 4) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg = AXP192_REG_GPIO0_CTRL + gpio_num;
    uint8_t value = level ? AXP192_GPIO_OUTPUT_HIGH : AXP192_GPIO_OUTPUT_LOW;
    
    return axp192_write_register(handle, reg, value);
}

esp_err_t axp192_set_lcd_power(axp192_handle_t handle, bool enable) {
    if (!handle || !handle->is_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting LCD power: %s", enable ? "ON" : "OFF");
    return axp192_modify_register(handle, AXP192_REG_EXTEN_DCDC2, AXP192_DCDC3_ENABLE, 
                                  enable ? AXP192_DCDC3_ENABLE : 0);
}

esp_err_t axp192_set_lcd_backlight(axp192_handle_t handle, bool enable) {
    if (!handle || !handle->is_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting LCD backlight: %s", enable ? "ON" : "OFF");
    // 根据原理图，LCD背光通过DCDC3控制，LDO2用于PERI_VDD供电
    // 注意：这里只控制DCDC3，不影响LDO2
    return axp192_modify_register(handle, AXP192_REG_EXTEN_DCDC2, AXP192_DCDC3_ENABLE, 
                                  enable ? AXP192_DCDC3_ENABLE : 0);
}

esp_err_t axp192_set_lcd_reset(axp192_handle_t handle, bool reset) {
    if (!handle || !handle->is_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting LCD reset: %s", reset ? "RESET" : "NORMAL");
    return axp192_set_gpio(handle, 4, !reset); // GPIO4低电平复位，根据原理图
}

axp192_config_t axp192_get_default_config(void) {
    axp192_config_t config = {
        .i2c_port = I2C_NUM_0,
        .sda_pin = GPIO_NUM_21,
        .scl_pin = GPIO_NUM_22,
        .i2c_freq = 100000,
        .i2c_addr = 0x34
    };
    return config;
}

esp_err_t axp192_set_dcdc1_voltage(axp192_handle_t handle, uint32_t voltage_mv) {
    if (!handle || !handle->is_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (voltage_mv < 700 || voltage_mv > 3500) {
        ESP_LOGE(TAG, "DCDC1电压超出范围: %d mV (范围: 700-3500 mV)", voltage_mv);
        return ESP_ERR_INVALID_ARG;
    }
    
    // 计算电压寄存器值：(V - 700mV) / 25mV
    uint8_t volt_code = (voltage_mv - 700) / 25;
    
    // 限制寄存器值范围（0x00-0x70，对应700mV-3500mV）
    if (volt_code > 0x70) {
        volt_code = 0x70;
    }
    
    ESP_LOGI(TAG, "设置DCDC1电压: %d mV (寄存器值: 0x%02X)", voltage_mv, volt_code);
    
    // 写入DCDC1电压寄存器（0x26）
    esp_err_t ret = axp192_write_register(handle, AXP192_REG_DCDC1_VOLTAGE, volt_code);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "DCDC1电压设置成功: %d mV", voltage_mv);
        
        // 验证设置是否成功
        vTaskDelay(pdMS_TO_TICKS(10));
        uint8_t verify_reg;
        if (axp192_read_register(handle, AXP192_REG_DCDC1_VOLTAGE, &verify_reg) == ESP_OK) {
            if (verify_reg == volt_code) {
                ESP_LOGI(TAG, "DCDC1电压验证成功: 0x%02X", verify_reg);
            } else {
                ESP_LOGW(TAG, "DCDC1电压验证失败: 期望0x%02X，实际0x%02X", volt_code, verify_reg);
            }
        }
    } else {
        ESP_LOGE(TAG, "DCDC1电压设置失败: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t axp192_enable_dcdc1(axp192_handle_t handle) {
    if (!handle || !handle->is_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 读取当前电源输出状态（0x12寄存器EXTEN_DCDC2）
    uint8_t power_reg;
    esp_err_t ret = axp192_read_register(handle, AXP192_REG_EXTEN_DCDC2, &power_reg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取电源输出寄存器失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "当前电源输出寄存器值: 0x%02X (DCDC1:%d DCDC3:%d LDO2:%d LDO3:%d)", 
             power_reg,
             (power_reg & AXP192_DCDC1_ENABLE) ? 1 : 0,
             (power_reg & AXP192_DCDC3_ENABLE) ? 1 : 0,
             (power_reg & AXP192_LDO2_ENABLE) ? 1 : 0,
             (power_reg & AXP192_LDO3_ENABLE) ? 1 : 0);
    
    // 检查DCDC1是否已使能
    if (power_reg & AXP192_DCDC1_ENABLE) {
        ESP_LOGI(TAG, "DCDC1已使能，无需重复设置");
        return ESP_OK;
    }
    
    // 置位DC-DC1使能位（bit0），保留其他位不变
    power_reg |= AXP192_DCDC1_ENABLE;
    
    ESP_LOGI(TAG, "使能DCDC1（新寄存器值: 0x%02X）", power_reg);
    
    // 写回寄存器
    ret = axp192_write_register(handle, AXP192_REG_EXTEN_DCDC2, power_reg);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "DCDC1使能成功");
        
        // 验证设置是否成功
        vTaskDelay(pdMS_TO_TICKS(10));
        uint8_t verify_reg;
        if (axp192_read_register(handle, AXP192_REG_EXTEN_DCDC2, &verify_reg) == ESP_OK) {
            if (verify_reg & AXP192_DCDC1_ENABLE) {
                ESP_LOGI(TAG, "DCDC1使能验证成功: 0x%02X", verify_reg);
            } else {
                ESP_LOGW(TAG, "DCDC1使能验证失败: 寄存器值0x%02X，bit0未置位", verify_reg);
            }
        }
    } else {
        ESP_LOGE(TAG, "DCDC1使能失败: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

// =============================================================================
// 新版I2C Master API实现
// =============================================================================

// 注意：已禁用新版I2C Master API支持（v2函数），只使用旧版API以避免驱动冲突
#if 0  // 禁用新版API支持
esp_err_t axp192_create_v2(i2c_master_dev_handle_t device_handle, axp192_handle_t* handle_out) {
    if (!device_handle || !handle_out) {
        ESP_LOGE(TAG, "参数无效：device_handle或handle_out为空");
        return ESP_ERR_INVALID_ARG;
    }

    // 分配句柄内存
    axp192_handle_t handle = (axp192_handle_t)malloc(sizeof(struct axp192_handle));
    if (!handle) {
        ESP_LOGE(TAG, "分配AXP192句柄内存失败");
        return ESP_ERR_NO_MEM;
    }

    // 初始化句柄
    memset(handle, 0, sizeof(struct axp192_handle));
    handle->device_handle = device_handle;
    handle->use_v2_api = true;
    handle->is_initialized = false;

    *handle_out = handle;
    ESP_LOGI(TAG, "AXP192句柄创建成功（新版I2C Master API）");
    return ESP_OK;
}

esp_err_t axp192_init_v2(axp192_handle_t handle) {
    if (!handle || !handle->device_handle) {
        ESP_LOGE(TAG, "参数无效：handle或device_handle为空");
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->is_initialized) {
        ESP_LOGW(TAG, "AXP192已初始化（新版API）");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "初始化AXP192（新版I2C Master API）...");

    // 测试AXP192通信
    uint8_t power_status;
    esp_err_t ret = axp192_read_register(handle, AXP192_REG_POWER_STATUS, &power_status);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "AXP192通信测试失败: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "AXP192通信测试成功，电源状态: 0x%02X", power_status);

    // 读取并显示当前EXTEN_DCDC2寄存器状态
    uint8_t exten_dcdc2_status;
    ret = axp192_read_register(handle, AXP192_REG_EXTEN_DCDC2, &exten_dcdc2_status);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "当前EXTEN_DCDC2寄存器: 0x%02X", exten_dcdc2_status);
    }

    // 初始化LCD电源和背光
    // 使能DCDC3 (LCD背光电源)
    ret = axp192_modify_register(handle, AXP192_REG_EXTEN_DCDC2, AXP192_DCDC3_ENABLE, AXP192_DCDC3_ENABLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "使能DCDC3失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 设置DCDC3电压为2.8V (0x14)
    ret = axp192_write_register(handle, AXP192_REG_DCDC3_VOLTAGE, 0x14);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "设置DCDC3电压失败: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "DCDC3 (LCD背光) 已使能并设置为2.8V");

    // 使能LDO2 (PERI_VDD，为LCD模块VDD供电)
    ret = axp192_modify_register(handle, AXP192_REG_EXTEN_DCDC2, AXP192_LDO2_ENABLE, AXP192_LDO2_ENABLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "使能LDO2失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 设置LDO2电压为3.3V (0x1F)
    ret = axp192_write_register(handle, AXP192_REG_LDO2_VOLTAGE, 0x1F);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "设置LDO2电压失败: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "LDO2 (PERI_VDD) 已使能并设置为3.3V");

    // 配置GPIO4为输出模式（LCD复位控制）
    ret = axp192_write_register(handle, AXP192_REG_GPIO4_CTRL, AXP192_GPIO_OUTPUT_HIGH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "配置GPIO4失败: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "GPIO4已配置为LCD复位控制");

    // 最终状态检查
    ret = axp192_read_register(handle, AXP192_REG_EXTEN_DCDC2, &exten_dcdc2_status);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "最终EXTEN_DCDC2寄存器: 0x%02X (DCDC1:%d DCDC3:%d LDO2:%d LDO3:%d)",
                 exten_dcdc2_status,
                 (exten_dcdc2_status & AXP192_DCDC1_ENABLE) ? 1 : 0,
                 (exten_dcdc2_status & AXP192_DCDC3_ENABLE) ? 1 : 0,
                 (exten_dcdc2_status & AXP192_LDO2_ENABLE) ? 1 : 0,
                 (exten_dcdc2_status & AXP192_LDO3_ENABLE) ? 1 : 0);
    }

    handle->is_initialized = true;
    ESP_LOGI(TAG, "AXP192初始化成功（新版I2C Master API）");
    return ESP_OK;
}
#endif  // 禁用新版API支持