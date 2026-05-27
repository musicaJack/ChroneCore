#include "chrone_haptic.h"

#include <stdbool.h>
#include <stdint.h>

#include "bsp/m5stack_core_2.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "vibration/vibration.h"

/** 短 detent 脉冲，避免长时间占用 AXP192 I2C 影响触摸采样 */
#define DETENT_US (28 * 1000)
/** 连拨时节流，每格最多一次触觉 */
#define DETENT_MIN_INTERVAL_US (48 * 1000)

static bool s_ready;
static esp_timer_handle_t s_off_timer;
static int64_t s_last_detent_us;

static void motor_off_cb(void *arg)
{
    (void)arg;
    (void)bsp_feature_enable(BSP_FEATURE_VIBRATION, false);
}

static void ensure_off_timer(void)
{
    if (s_off_timer) {
        return;
    }
    const esp_timer_create_args_t args = {
        .callback = motor_off_cb,
        .name = "chrone_haptic",
    };
    (void)esp_timer_create(&args, &s_off_timer);
}

static void haptic_reset_state(void)
{
    if (s_off_timer) {
        (void)esp_timer_stop(s_off_timer);
    }
    (void)bsp_feature_enable(BSP_FEATURE_VIBRATION, false);
}

static void ensure_init(void)
{
    if (s_ready) {
        return;
    }
    if (vibration_init() == ESP_OK) {
        s_ready = true;
        ensure_off_timer();
    }
}

void chrone_haptic_prepare(void)
{
    ensure_init();
}

void chrone_haptic_detent(void)
{
    ensure_init();
    if (!s_ready || !s_off_timer) {
        return;
    }

    const int64_t now = esp_timer_get_time();
    if (now - s_last_detent_us < DETENT_MIN_INTERVAL_US) {
        return;
    }
    s_last_detent_us = now;

    (void)esp_timer_stop(s_off_timer);
    (void)bsp_feature_enable(BSP_FEATURE_VIBRATION, true);
    (void)esp_timer_start_once(s_off_timer, DETENT_US);
}

void chrone_haptic_confirm(void)
{
    ensure_init();
    if (!s_ready) {
        return;
    }
    haptic_reset_state();
    (void)vibration_trigger();
}
