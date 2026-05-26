#include "chrone_app.hpp"

#include "chrone_display.hpp"
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
    create_clock_ui();

    if (xTaskCreate(wifi_background_task, "chrone_wifi", 8192, nullptr, 3, nullptr) != pdPASS) {
        ESP_LOGE(TAG, "wifi task create failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "clock UI + WiFi task started");
    return ESP_OK;
}
