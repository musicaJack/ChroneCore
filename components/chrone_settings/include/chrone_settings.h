#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CHRONE_SETTINGS_BRIGHTNESS_MIN  10
#define CHRONE_SETTINGS_BRIGHTNESS_MAX  100
#define CHRONE_SETTINGS_BRIGHTNESS_DEFAULT 60

#define CHRONE_SETTINGS_BLANK_TIMEOUT_DEFAULT_S 120

#define CHRONE_SETTINGS_ALARM_VOL_DEFAULT 68
#define CHRONE_SETTINGS_VIBE_LEVEL_DEFAULT 2
#define CHRONE_SETTINGS_VIBE_LEVEL_MAX 3

esp_err_t chrone_settings_init(void);

uint8_t chrone_settings_get_brightness(void);
esp_err_t chrone_settings_set_brightness(uint8_t percent);

uint32_t chrone_settings_get_blank_timeout_s(void);
esp_err_t chrone_settings_set_blank_timeout_s(uint32_t seconds);

uint8_t chrone_settings_get_alarm_vol(void);
esp_err_t chrone_settings_set_alarm_vol(uint8_t percent);

uint8_t chrone_settings_get_vibe_level(void);
esp_err_t chrone_settings_set_vibe_level(uint8_t level);

/** 0=Off, 1=Weak, 2=Med, 3=Strong */
const char *chrone_settings_vibe_label(uint8_t level);

#ifdef __cplusplus
}
#endif
