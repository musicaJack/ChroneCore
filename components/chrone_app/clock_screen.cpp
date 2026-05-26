#include "chrone_app.hpp"

#include "chrone_display.hpp"
#include "chrone_ui.h"

#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "chrone_app";

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
}

extern "C" esp_err_t chrone_app_start(void)
{
    create_clock_ui();
    ESP_LOGI(TAG, "clock UI started (LVGL 1 Hz partial refresh)");
    return ESP_OK;
}
