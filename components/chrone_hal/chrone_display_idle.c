#include "chrone_display_idle.h"

#include "chrone_hal.h"
#include "chrone_settings.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "display_idle";

static bool s_display_off;
static uint32_t s_last_touch_ms;

static uint32_t now_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void chrone_display_idle_init(void)
{
    s_display_off = false;
    s_last_touch_ms = now_ms();
}

void chrone_display_idle_wake(void)
{
    const uint8_t b = chrone_settings_get_brightness();
    chrone_hal_touch_lvgl_enable(true);
    if (chrone_hal_set_brightness(b) == ESP_OK) {
        s_display_off = false;
        s_last_touch_ms = now_ms();
    }
}

void chrone_display_idle_on_touch(void)
{
    s_last_touch_ms = now_ms();
    if (s_display_off) {
        chrone_display_idle_wake();
    }
}

void chrone_display_idle_tick(bool clock_screen_active)
{
    if (!clock_screen_active || s_display_off) {
        return;
    }

    const uint32_t elapsed = now_ms() - s_last_touch_ms;
    const uint32_t limit_ms = chrone_settings_get_blank_timeout_s() * 1000U;
    if (elapsed >= limit_ms) {
        if (chrone_hal_backlight_off() == ESP_OK) {
            s_display_off = true;
            chrone_hal_touch_lvgl_enable(false);
            ESP_LOGI(TAG, "backlight dim (idle %lus)", (unsigned long)chrone_settings_get_blank_timeout_s());
        }
    }
}

bool chrone_display_idle_is_off(void)
{
    return s_display_off;
}
