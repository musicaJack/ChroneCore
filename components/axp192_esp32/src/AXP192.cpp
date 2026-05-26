/**
 * @file AXP192.cpp
 * @brief AXP192电源管理芯片C++类实现
 */

#include "axp192_esp32/AXP192.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "AXP192";

namespace AXP192 {

AXP192::AXP192(I2CManager::I2CBusManager* bus_manager, const AXP192Config& config)
    : bus_manager_(bus_manager), config_(config), device_handle_(nullptr), initialized_(false) {
    if (!bus_manager_) {
        ESP_LOGE(TAG, "I2C总线管理器不能为空");
    }
}

AXP192::~AXP192() {
    deinit();
}

esp_err_t AXP192::init() {
    if (initialized_) {
        ESP_LOGW(TAG, "AXP192已初始化");
        return ESP_OK;
    }

    if (!bus_manager_) {
        ESP_LOGE(TAG, "I2C总线管理器未设置");
        return ESP_ERR_INVALID_STATE;
    }

    if (!bus_manager_->isInitialized()) {
        ESP_LOGE(TAG, "I2C总线未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "初始化AXP192 (地址: 0x%02X)", config_.i2c_addr);

    // 添加I2C设备到总线
    I2CManager::I2CDeviceConfig device_config = {
        .device_addr = config_.i2c_addr,
        .clk_speed_hz = config_.i2c_clk_speed_hz,
        .disable_ack_check = false,
    };

    esp_err_t ret = bus_manager_->addDevice(device_config, &device_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "添加AXP192设备失败: %s", esp_err_to_name(ret));
        device_handle_ = nullptr;
        return ret;
    }

    // 测试AXP192通信
    uint8_t power_status;
    ret = readRegister(Registers::POWER_STATUS, &power_status);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "AXP192通信测试失败: %s", esp_err_to_name(ret));
        bus_manager_->removeDevice(device_handle_);
        device_handle_ = nullptr;
        return ret;
    }

    ESP_LOGI(TAG, "AXP192通信测试成功，电源状态: 0x%02X", power_status);

    // 读取并显示当前EXTEN_DCDC2寄存器状态
    uint8_t exten_dcdc2_status;
    ret = readRegister(Registers::EXTEN_DCDC2, &exten_dcdc2_status);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "当前EXTEN_DCDC2寄存器: 0x%02X", exten_dcdc2_status);
    }

    // 配置LCD电源和背光
    ESP_LOGI(TAG, "配置LCD电源和背光...");
    
    // 使能DCDC3（LCD背光，2.8V）
    ret = modifyRegister(Registers::DCDC3_VOLTAGE, 0x7F, 0x14); // 2.8V
    if (ret == ESP_OK) {
        ret = modifyRegister(Registers::EXTEN_DCDC2, PowerBits::DCDC3_ENABLE, PowerBits::DCDC3_ENABLE);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "DCDC3 (LCD backlight) enabled and set to 2.8V");
        }
    }

    // 使能LDO2（PERI_VDD，3.3V）
    ret = modifyRegister(Registers::LDO2_VOLTAGE, 0xF0, 0x1F << 4); // 3.3V
    if (ret == ESP_OK) {
        ret = modifyRegister(Registers::EXTEN_DCDC2, PowerBits::LDO2_ENABLE, PowerBits::LDO2_ENABLE);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "LDO2 (PERI_VDD) enabled and set to 3.3V for LCD VDD");
        }
    }

    // 配置GPIO4用于LCD复位控制
    ret = writeRegister(Registers::GPIO4_CTRL, 0x01); // GPIO4输出模式
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "GPIO4 configured for LCD reset control");
    }

    initialized_ = true;
    ESP_LOGI(TAG, "AXP192初始化成功");
    return ESP_OK;
}

esp_err_t AXP192::deinit() {
    if (!initialized_ || !device_handle_ || !bus_manager_) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "反初始化AXP192");
    esp_err_t ret = bus_manager_->removeDevice(device_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "移除AXP192设备失败: %s", esp_err_to_name(ret));
    }

    device_handle_ = nullptr;
    initialized_ = false;
    ESP_LOGI(TAG, "AXP192反初始化成功");
    return ESP_OK;
}

esp_err_t AXP192::readRegister(uint8_t reg, uint8_t* value) {
    if (!initialized_ || !device_handle_ || !value) {
        return ESP_ERR_INVALID_STATE;
    }

    return I2CManager::I2CBusManager::readRegister(device_handle_, reg, value);
}

esp_err_t AXP192::writeRegister(uint8_t reg, uint8_t value) {
    if (!initialized_ || !device_handle_) {
        return ESP_ERR_INVALID_STATE;
    }

    return I2CManager::I2CBusManager::writeRegister(device_handle_, reg, value);
}

esp_err_t AXP192::modifyRegister(uint8_t reg, uint8_t mask, uint8_t value) {
    if (!initialized_ || !device_handle_) {
        return ESP_ERR_INVALID_STATE;
    }

    return I2CManager::I2CBusManager::modifyRegister(device_handle_, reg, mask, value);
}

esp_err_t AXP192::setGPIO(uint8_t gpio_num, bool level) {
    if (!initialized_ || !device_handle_ || gpio_num > 4) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg = Registers::GPIO0_CTRL + gpio_num;
    uint8_t value = level ? 0x01 : 0x00;
    return writeRegister(reg, value);
}

esp_err_t AXP192::setLCDPower(bool enable) {
    ESP_LOGI(TAG, "设置LCD电源: %s", enable ? "ON" : "OFF");
    uint8_t mask = PowerBits::LDO2_ENABLE;
    uint8_t value = enable ? PowerBits::LDO2_ENABLE : 0;
    return modifyRegister(Registers::EXTEN_DCDC2, mask, value);
}

esp_err_t AXP192::setLCDBacklight(bool enable) {
    ESP_LOGI(TAG, "设置LCD背光: %s", enable ? "ON" : "OFF");
    uint8_t mask = PowerBits::DCDC3_ENABLE;
    uint8_t value = enable ? PowerBits::DCDC3_ENABLE : 0;
    return modifyRegister(Registers::EXTEN_DCDC2, mask, value);
}

esp_err_t AXP192::setLCDReset(bool reset) {
    ESP_LOGI(TAG, "设置LCD复位: %s", reset ? "RESET" : "NORMAL");
    return setGPIO(4, !reset);  // GPIO4低电平复位
}

esp_err_t AXP192::setDCDC1Voltage(uint32_t voltage_mv) {
    if (!initialized_ || !device_handle_) {
        return ESP_ERR_INVALID_STATE;
    }

    if (voltage_mv < 700 || voltage_mv > 3500) {
        ESP_LOGE(TAG, "DC-DC1电压超出范围: %lu mV (范围: 700-3500 mV)", voltage_mv);
        return ESP_ERR_INVALID_ARG;
    }

    // 计算电压寄存器值：(V - 700mV) / 25mV
    uint8_t volt_code = (voltage_mv - 700) / 25;
    ESP_LOGI(TAG, "设置DCDC1电压: %lu mV (寄存器值: 0x%02X)", voltage_mv, volt_code);

    esp_err_t ret = writeRegister(Registers::DCDC1_VOLTAGE, volt_code);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "设置DCDC1电压失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 验证电压设置
    uint8_t verify_value;
    ret = readRegister(Registers::DCDC1_VOLTAGE, &verify_value);
    if (ret == ESP_OK && verify_value == volt_code) {
        ESP_LOGI(TAG, "DCDC1电压验证成功: 0x%02X", verify_value);
    } else {
        ESP_LOGW(TAG, "DCDC1电压验证失败: 期望0x%02X，实际0x%02X", volt_code, verify_value);
    }

    return ret;
}

esp_err_t AXP192::enableDCDC1() {
    if (!initialized_ || !device_handle_) {
        return ESP_ERR_INVALID_STATE;
    }

    // 读取当前电源输出状态（0x12寄存器EXTEN_DCDC2）
    uint8_t exten_dcdc2_status;
    esp_err_t ret = readRegister(Registers::EXTEN_DCDC2, &exten_dcdc2_status);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取EXTEN_DCDC2寄存器失败: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "当前电源输出寄存器值: 0x%02X (DCDC1:%d DCDC3:%d LDO2:%d LDO3:%d)",
             exten_dcdc2_status,
             (exten_dcdc2_status & PowerBits::DCDC1_ENABLE) ? 1 : 0,
             (exten_dcdc2_status & PowerBits::DCDC3_ENABLE) ? 1 : 0,
             (exten_dcdc2_status & PowerBits::LDO2_ENABLE) ? 1 : 0,
             (exten_dcdc2_status & PowerBits::LDO3_ENABLE) ? 1 : 0);

    // 检查DCDC1是否已使能
    if (exten_dcdc2_status & PowerBits::DCDC1_ENABLE) {
        ESP_LOGI(TAG, "DCDC1已使能，无需重复设置");
        return ESP_OK;
    }

    // 使能DCDC1（设置bit0）
    ret = modifyRegister(Registers::EXTEN_DCDC2, PowerBits::DCDC1_ENABLE, PowerBits::DCDC1_ENABLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "使能DCDC1失败: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "DCDC1已使能");
    return ESP_OK;
}

} // namespace AXP192

