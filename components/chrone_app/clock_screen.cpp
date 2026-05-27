#include "chrone_app.hpp"

#include "chrone_alarm.h"
#include "chrone_audio.h"
#include "chrone_display.hpp"
#include "chrone_hal.h"
#include "chrone_imu.h"
#include "chrone_input.h"
#include "chrone_haptic.h"
#include "chrone_ui.h"
#include "chrone_wifi.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "chrone_app";

static void wifi_background_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "WiFi background task start");
    chrone_wifi_start();
}

static void create_clock_ui(void)
{
    chrone::DisplayLock lock;
    if (!lock.ok()) {
        return;
    }

    lv_obj_t *screen = lv_scr_act();
    if (chrone_ui_init(screen) != ESP_OK) {
        ESP_LOGE(TAG, "chrone_ui_init failed");
        return;
    }
    chrone_ui_refresh();
    chrone_ui_poll_services();
}

extern "C" esp_err_t chrone_app_start(void)
{
    ESP_ERROR_CHECK(chrone_audio_init());
    ESP_ERROR_CHECK(chrone_imu_init());
    chrone_haptic_prepare();
    chrone_alarm_load();

    create_clock_ui();

    esp_err_t ret = chrone_input_init(chrone_hal_get_touch());
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "chrone_input_init: %s", esp_err_to_name(ret));
    }
    ESP_ERROR_CHECK(chrone_app_poll_init());

    if (xTaskCreate(wifi_background_task, "chrone_wifi", 8192, nullptr, 3, nullptr) != pdPASS) {
        ESP_LOGE(TAG, "wifi task create failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "clock UI + WiFi task started");
    return ESP_OK;
}
