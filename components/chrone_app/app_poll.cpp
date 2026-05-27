#include "chrone_app.hpp"

#include "chrone_alarm.h"
#include "chrone_display.hpp"
#include "chrone_hal.h"
#include "chrone_imu.h"
#include "chrone_input.h"
#include "chrone_time.h"
#include "chrone_ui.h"

#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "chrone_app";
static lv_timer_t *s_input_timer;
static uint32_t s_alarm_tick_bucket = UINT32_MAX;
static bool s_ring_ptr_was_down;

static bool lvgl_pointer_down(void)
{
    lv_indev_t *indev = lv_indev_get_next(nullptr);
    while (indev) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            const lv_indev_state_t st = lv_indev_get_state(indev);
            if (st == LV_INDEV_STATE_PRESSED) {
                return true;
            }
        }
        indev = lv_indev_get_next(indev);
    }
    return false;
}

/** 1 Hz 闹钟调度（不依赖时钟屏 tick，配置界面保存后也能触发） */
static void alarm_schedule_tick(void)
{
    struct tm tm_local;
    if (chrone_time_get_local_tm(&tm_local) != ESP_OK) {
        static uint32_t s_last_time_fail_log_ms;
        const uint32_t now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        if (now_ms - s_last_time_fail_log_ms >= 60000U) {
            s_last_time_fail_log_ms = now_ms;
            ESP_LOGW(TAG, "alarm_schedule_tick: chrone_time_get_local_tm failed");
        }
        return;
    }

    const uint32_t bucket = (uint32_t)(tm_local.tm_mday * 86400U + tm_local.tm_hour * 3600U
                                       + tm_local.tm_min * 60U + tm_local.tm_sec);
    if (bucket == s_alarm_tick_bucket) {
        return;
    }
    s_alarm_tick_bucket = bucket;

    chrone_alarm_check_tick(&tm_local);
}

static void input_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    alarm_schedule_tick();
    chrone_alarm_poll();

    {
        chrone::DisplayLock lock;
        if (lock.ok()) {
            chrone_input_poll();
        }
    }

    if (chrone_alarm_get_state() == CHRONE_ALARM_STATE_RINGING) {
        if (chrone_imu_shake_detected()) {
            chrone_alarm_dismiss();
            s_ring_ptr_was_down = false;
            return;
        }

        const bool down = lvgl_pointer_down();
        chrone::DisplayLock lock;
        if (lock.ok()) {
            if (down && !s_ring_ptr_was_down) {
                chrone_alarm_dismiss();
                ESP_LOGI(TAG, "alarm dismissed (touch)");
            }
            s_ring_ptr_was_down = down;
            chrone_ui_update_ringing();
        }
        return;
    }

    s_ring_ptr_was_down = false;

    if (chrone_ui_alarm_config_active()) {
        static bool s_mid_prev;
        const bool mid = chrone_input_middle_down();
        const bool edge = mid && !s_mid_prev;
        s_mid_prev = mid;
        if (edge) {
            chrone::DisplayLock lock;
            if (lock.ok()) {
                chrone_ui_show_clock();
                ESP_LOGI(TAG, "exit alarm config (middle)");
            }
        }
    }
}

extern "C" esp_err_t chrone_app_poll_init(void)
{
    if (s_input_timer) {
        return ESP_OK;
    }
    s_input_timer = lv_timer_create(input_timer_cb, 50, nullptr);
    if (!s_input_timer) {
        return ESP_FAIL;
    }
    lv_timer_set_repeat_count(s_input_timer, -1);
    ESP_LOGI(TAG, "input poll timer 50ms");
    return ESP_OK;
}
