#include "chrone_imu.h"

#include "esp_log.h"

static const char *TAG = "chrone_imu";

esp_err_t chrone_imu_init(void)
{
    ESP_LOGI(TAG, "IMU stub (MPU6886 shake TBD)");
    return ESP_OK;
}

bool chrone_imu_shake_detected(void)
{
    return false;
}
