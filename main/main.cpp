#include "chrone_app.hpp"
#include "chrone_hal.h"
#include "chrone_time.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include <sys/time.h>

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ChroneCore v0.1.0 (hybrid C/C++)");
    ESP_LOGI(TAG, "========================================");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(chrone_hal_init());

    setenv("TZ", "CST-8", 1);
    tzset();

    ret = chrone_time_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "RTC init failed (%s); clock UI uses system time until RTC/SNTP",
                 esp_err_to_name(ret));
    }

    ESP_ERROR_CHECK(chrone_app_start());

    ESP_LOGI(TAG, "ChroneCore started");
}
