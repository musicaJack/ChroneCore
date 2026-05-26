/**
 * @file AXP192.hpp
 * @brief AXP192电源管理芯片C++类（使用新版I2C Master API）
 * @description 基于I2CBusManager的AXP192封装类
 */

#pragma once

#include "i2c_manager/I2CBusManager.hpp"
#include "esp_err.h"
#include <cstdint>

#ifdef __cplusplus

namespace AXP192 {

/**
 * @brief AXP192寄存器定义
 */
namespace Registers {
    constexpr uint8_t POWER_STATUS = 0x00;
    constexpr uint8_t POWER_MODE = 0x01;
    constexpr uint8_t EXTEN_DCDC2 = 0x12;      // 电源输出使能寄存器
    constexpr uint8_t DCDC1_VOLTAGE = 0x26;    // DC-DC1电压设置寄存器
    constexpr uint8_t DCDC2_VOLTAGE = 0x23;
    constexpr uint8_t DCDC3_VOLTAGE = 0x27;
    constexpr uint8_t LDO2_VOLTAGE = 0x28;
    constexpr uint8_t LDO3_VOLTAGE = 0x29;
    constexpr uint8_t GPIO0_CTRL = 0x90;
    constexpr uint8_t GPIO1_CTRL = 0x91;
    constexpr uint8_t GPIO2_CTRL = 0x92;
    constexpr uint8_t GPIO3_CTRL = 0x93;
    constexpr uint8_t GPIO4_CTRL = 0x94;
}

/**
 * @brief AXP192电源控制位
 */
namespace PowerBits {
    constexpr uint8_t DCDC1_ENABLE = 0x01;  // DC-DC1使能位（bit0）
    constexpr uint8_t DCDC3_ENABLE = 0x02;  // DC-DC3使能位（bit1，LCD背光）
    constexpr uint8_t LDO2_ENABLE = 0x04;   // LDO2使能位（bit2）
    constexpr uint8_t LDO3_ENABLE = 0x08;   // LDO3使能位（bit3）
}

/**
 * @brief AXP192配置结构
 */
struct AXP192Config {
    uint8_t i2c_addr = 0x34;           // I2C地址（默认0x34）
    uint32_t i2c_clk_speed_hz = 400000; // I2C时钟速度（默认400kHz）
};

/**
 * @brief AXP192 C++类
 * @description 使用新版I2C Master API的AXP192封装
 */
class AXP192 {
public:
    /**
     * @brief 构造函数
     * @param bus_manager I2C总线管理器（共享总线）
     * @param config AXP192配置
     */
    explicit AXP192(I2CManager::I2CBusManager* bus_manager, 
                    const AXP192Config& config = AXP192Config());

    /**
     * @brief 析构函数（自动清理资源）
     */
    ~AXP192();

    // 禁用拷贝构造和赋值
    AXP192(const AXP192&) = delete;
    AXP192& operator=(const AXP192&) = delete;

    /**
     * @brief 初始化AXP192
     * @return ESP_OK成功，其他值失败
     */
    esp_err_t init();

    /**
     * @brief 反初始化AXP192
     * @return ESP_OK成功，其他值失败
     */
    esp_err_t deinit();

    /**
     * @brief 检查是否已初始化
     * @return true已初始化，false未初始化
     */
    bool isInitialized() const { return initialized_ && device_handle_ != nullptr; }

    /**
     * @brief 读取寄存器
     * @param reg 寄存器地址
     * @param value 输出寄存器值
     * @return ESP_OK成功，其他值失败
     */
    esp_err_t readRegister(uint8_t reg, uint8_t* value);

    /**
     * @brief 写入寄存器
     * @param reg 寄存器地址
     * @param value 寄存器值
     * @return ESP_OK成功，其他值失败
     */
    esp_err_t writeRegister(uint8_t reg, uint8_t value);

    /**
     * @brief 修改寄存器位（读取-修改-写入）
     * @param reg 寄存器地址
     * @param mask 位掩码
     * @param value 新值（只使用mask指定的位）
     * @return ESP_OK成功，其他值失败
     */
    esp_err_t modifyRegister(uint8_t reg, uint8_t mask, uint8_t value);

    /**
     * @brief 设置GPIO输出状态
     * @param gpio_num GPIO编号 (0-4)
     * @param level 输出电平 (false=低电平, true=高电平)
     * @return ESP_OK成功，其他值失败
     */
    esp_err_t setGPIO(uint8_t gpio_num, bool level);

    /**
     * @brief 设置LCD电源
     * @param enable 是否使能LCD电源
     * @return ESP_OK成功，其他值失败
     */
    esp_err_t setLCDPower(bool enable);

    /**
     * @brief 设置LCD背光
     * @param enable 是否使能LCD背光
     * @return ESP_OK成功，其他值失败
     */
    esp_err_t setLCDBacklight(bool enable);

    /**
     * @brief 设置LCD复位
     * @param reset 是否复位LCD
     * @return ESP_OK成功，其他值失败
     */
    esp_err_t setLCDReset(bool reset);

    /**
     * @brief 设置DC-DC1输出电压（为MPU6886供电）
     * @param voltage_mv 输出电压（单位：mV，范围700~3500，步长25mV）
     * @return ESP_OK成功，其他值失败
     */
    esp_err_t setDCDC1Voltage(uint32_t voltage_mv);

    /**
     * @brief 使能DC-DC1输出
     * @return ESP_OK成功，其他值失败
     */
    esp_err_t enableDCDC1();

    /**
     * @brief 获取I2C设备句柄（用于直接访问）
     * @return I2C设备句柄，未初始化返回nullptr
     */
    i2c_master_dev_handle_t getDeviceHandle() const { return device_handle_; }

private:
    I2CManager::I2CBusManager* bus_manager_;  // I2C总线管理器（不拥有所有权）
    AXP192Config config_;                      // 配置
    i2c_master_dev_handle_t device_handle_;    // I2C设备句柄
    bool initialized_;                         // 是否已初始化
};

} // namespace AXP192

#endif // __cplusplus

