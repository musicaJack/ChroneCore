/**
 * BM8563 RTC driver for M5Stack Core2 (I2C 0x51, GPIO21/22).
 * Ported from Core2-for-AWS-IoT-Kit (uses I2C_NUM_1). Bus is initialized by
 * BSP via bsp_i2c_init() before first use (e.g. chrone_hal display init).
 */

#include "bm8563.h"

#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <string.h>

static const char *TAG = "bm8563";

/* Must match CONFIG_BSP_I2C_NUM (M5Stack BSP default: 1, not 0). */
#define BM8563_I2C_PORT ((i2c_port_t)CONFIG_BSP_I2C_NUM)

static bool s_initialized;

static uint8_t byte2bcd(uint8_t v)
{
    return (uint8_t)(((v / 10) << 4) + (v % 10));
}

static uint8_t bcd2byte(uint8_t v)
{
    return (uint8_t)((v >> 4) * 10 + (v & 0x0f));
}

static esp_err_t i2c_write_reg(uint8_t reg, const uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BM8563_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    if (len > 0) {
        i2c_master_write(cmd, (uint8_t *)data, len, true);
    }
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(BM8563_I2C_PORT, cmd, pdMS_TO_TICKS(200));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t i2c_write_u8(uint8_t reg, uint8_t data)
{
    return i2c_write_reg(reg, &data, 1);
}

static esp_err_t i2c_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BM8563_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BM8563_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(BM8563_I2C_PORT, cmd, pdMS_TO_TICKS(200));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t bm8563_init(void)
{
    esp_err_t ret = i2c_write_u8(0x00, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "init reg 0x00: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = i2c_write_u8(0x01, 0x00);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = i2c_write_u8(0x0D, 0x00);
    if (ret != ESP_OK) {
        return ret;
    }
    s_initialized = true;
    ESP_LOGI(TAG, "BM8563 ready (I2C port %d)", BM8563_I2C_PORT);
    return ESP_OK;
}

esp_err_t bm8563_get_time(bm8563_datetime_t *out)
{
    if (!out || !s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t buf[7];
    esp_err_t ret = i2c_read_reg(0x02, buf, sizeof(buf));
    if (ret != ESP_OK) {
        return ret;
    }
    out->second = bcd2byte(buf[0] & 0x7f);
    out->minute = bcd2byte(buf[1] & 0x7f);
    out->hour = bcd2byte(buf[2] & 0x3f);
    out->mday = bcd2byte(buf[3] & 0x3f);
    out->wday = bcd2byte(buf[4] & 0x07);
    out->month = bcd2byte(buf[5] & 0x1f);
    out->year = (uint16_t)(bcd2byte(buf[6]) + ((buf[5] & 0x80) ? 1900 : 2000));
    return ESP_OK;
}

esp_err_t bm8563_set_time(const bm8563_datetime_t *in)
{
    if (!in || !s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t buf[7];
    buf[0] = byte2bcd(in->second);
    buf[1] = byte2bcd(in->minute);
    buf[2] = byte2bcd(in->hour);
    buf[3] = byte2bcd(in->mday);
    buf[4] = byte2bcd(in->wday & 0x07);
    buf[5] = byte2bcd(in->month) | (in->year >= 2000 ? 0x00 : 0x80);
    buf[6] = byte2bcd((uint8_t)(in->year % 100));
    return i2c_write_reg(0x02, buf, sizeof(buf));
}
