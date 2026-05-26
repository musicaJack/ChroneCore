/**
 * @file axp192.h
 * @brief AXP192 power management chip driver for ESP32
 * @description Driver for AXP192 power management chip used in M5Stack modules
 */

#ifndef AXP192_H
#define AXP192_H

#include "esp_err.h"
#include "driver/i2c.h"
#include "driver/i2c_master.h"  // 新版I2C Master API
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// AXP192寄存器定义
#define AXP192_REG_POWER_STATUS      0x00
#define AXP192_REG_POWER_MODE        0x01
#define AXP192_REG_EXTEN_DCDC2       0x12    // 电源输出使能寄存器（控制DCDC1/DCDC2/DCDC3/LDO2/LDO3）
#define AXP192_REG_DCDC1_VOLTAGE     0x26    // DC-DC1电压设置寄存器（为MPU6886供电）
#define AXP192_REG_DCDC2_VOLTAGE     0x23
#define AXP192_REG_DCDC3_VOLTAGE     0x27
#define AXP192_REG_LDO2_VOLTAGE      0x28
#define AXP192_REG_LDO3_VOLTAGE      0x29
#define AXP192_REG_GPIO0_CTRL        0x90
#define AXP192_REG_GPIO1_CTRL        0x91
#define AXP192_REG_GPIO2_CTRL        0x92
#define AXP192_REG_GPIO3_CTRL        0x93
#define AXP192_REG_GPIO4_CTRL        0x94

// AXP192 GPIO控制值
#define AXP192_GPIO_OUTPUT_LOW       0x00
#define AXP192_GPIO_OUTPUT_HIGH      0x01
#define AXP192_GPIO_INPUT            0x02

// AXP192电源控制位（在0x12寄存器EXTEN_DCDC2中）
#define AXP192_DCDC1_ENABLE          0x01    // DC-DC1使能位（bit0，为MPU6886供电）
#define AXP192_DCDC3_ENABLE          0x02    // DC-DC3使能位（bit1，LCD背光）
#define AXP192_LDO2_ENABLE           0x04    // LDO2使能位（bit2）
#define AXP192_LDO3_ENABLE           0x08    // LDO3使能位（bit3）
// 注意：DCDC2可能由其他寄存器控制，不在0x12寄存器中

// AXP192句柄类型
typedef struct axp192_handle* axp192_handle_t;

// AXP192配置结构
typedef struct {
    i2c_port_t i2c_port;
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
    uint32_t i2c_freq;
    uint8_t i2c_addr;
} axp192_config_t;

/**
 * @brief 创建AXP192句柄
 * @param config AXP192配置
 * @param handle_out 输出句柄
 * @return ESP_OK on success
 */
esp_err_t axp192_create(const axp192_config_t* config, axp192_handle_t* handle_out);

/**
 * @brief 销毁AXP192句柄
 * @param handle AXP192句柄
 * @return ESP_OK on success
 */
esp_err_t axp192_destroy(axp192_handle_t handle);

/**
 * @brief 初始化AXP192
 * @param handle AXP192句柄
 * @return ESP_OK on success
 */
esp_err_t axp192_init(axp192_handle_t handle);

/**
 * @brief 设置GPIO输出状态
 * @param handle AXP192句柄
 * @param gpio_num GPIO编号 (0-4)
 * @param level 输出电平 (0=低电平, 1=高电平)
 * @return ESP_OK on success
 */
esp_err_t axp192_set_gpio(axp192_handle_t handle, uint8_t gpio_num, bool level);

/**
 * @brief 设置LCD电源
 * @param handle AXP192句柄
 * @param enable 是否使能LCD电源
 * @return ESP_OK on success
 */
esp_err_t axp192_set_lcd_power(axp192_handle_t handle, bool enable);

/**
 * @brief 设置LCD背光
 * @param handle AXP192句柄
 * @param enable 是否使能LCD背光
 * @return ESP_OK on success
 */
esp_err_t axp192_set_lcd_backlight(axp192_handle_t handle, bool enable);

/**
 * @brief 设置LCD复位
 * @param handle AXP192句柄
 * @param reset 是否复位LCD
 * @return ESP_OK on success
 */
esp_err_t axp192_set_lcd_reset(axp192_handle_t handle, bool reset);

/**
 * @brief 获取默认配置
 * @return 默认配置结构
 */
axp192_config_t axp192_get_default_config(void);

/**
 * @brief 设置DC-DC1输出电压（为MPU6886供电）
 * @param handle AXP192句柄
 * @param voltage_mv 输出电压（单位：mV，范围700~3500，步长25mV）
 * @return ESP_OK成功，其他值失败
 * @note DC-DC1用于为MPU6886 IMU传感器供电，标准电压为3.3V（3300mV）
 */
esp_err_t axp192_set_dcdc1_voltage(axp192_handle_t handle, uint32_t voltage_mv);

/**
 * @brief 使能DC-DC1输出
 * @param handle AXP192句柄
 * @return ESP_OK成功，其他值失败
 * @note 使能DC-DC1后，MPU6886才能获得电源供电
 */
esp_err_t axp192_enable_dcdc1(axp192_handle_t handle);

// =============================================================================
// 新版I2C Master API支持（使用i2c_master_dev_handle_t）
// =============================================================================

/**
 * @brief 使用新版I2C Master API创建AXP192句柄
 * @param device_handle I2C设备句柄（通过i2c_master_bus_add_device创建）
 * @param handle_out 输出句柄
 * @return ESP_OK成功，其他值失败
 * @note 此函数使用新版I2C Master API，不需要单独初始化I2C总线
 */
esp_err_t axp192_create_v2(i2c_master_dev_handle_t device_handle, axp192_handle_t* handle_out);

/**
 * @brief 使用新版I2C Master API初始化AXP192（无需调用axp192_init）
 * @param handle AXP192句柄（通过axp192_create_v2创建）
 * @return ESP_OK成功，其他值失败
 * @note 使用新版API时，I2C总线应该已经初始化并添加了设备
 */
esp_err_t axp192_init_v2(axp192_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // AXP192_H
