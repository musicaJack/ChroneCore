#include "chrone_settings.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "chrone_settings";
static const char *NS = "chrone";

static uint8_t s_brightness = CHRONE_SETTINGS_BRIGHTNESS_DEFAULT;
static uint32_t s_blank_timeout_s = CHRONE_SETTINGS_BLANK_TIMEOUT_DEFAULT_S;
static uint8_t s_alarm_vol = CHRONE_SETTINGS_ALARM_VOL_DEFAULT;
static uint8_t s_vibe_level = CHRONE_SETTINGS_VIBE_LEVEL_DEFAULT;

static uint8_t clamp_u8(uint8_t v, uint8_t lo, uint8_t hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static esp_err_t load_u8(nvs_handle_t h, const char *key, uint8_t *out, uint8_t def)
{
    uint8_t v = def;
    if (nvs_get_u8(h, key, &v) == ESP_OK) {
        *out = v;
    }
    return ESP_OK;
}

static esp_err_t load_u32(nvs_handle_t h, const char *key, uint32_t *out, uint32_t def)
{
    uint32_t v = def;
    if (nvs_get_u32(h, key, &v) == ESP_OK) {
        *out = v;
    }
    return ESP_OK;
}

static esp_err_t commit_u8(nvs_handle_t h, const char *key, uint8_t v)
{
    esp_err_t ret = nvs_set_u8(h, key, v);
    if (ret == ESP_OK) {
        ret = nvs_commit(h);
    }
    return ret;
}

static esp_err_t commit_u32(nvs_handle_t h, const char *key, uint32_t v)
{
    esp_err_t ret = nvs_set_u32(h, key, v);
    if (ret == ESP_OK) {
        ret = nvs_commit(h);
    }
    return ret;
}

esp_err_t chrone_settings_init(void)
{
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NS, NVS_READONLY, &h);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open: %s, using defaults", esp_err_to_name(ret));
        return ESP_OK;
    }

    (void)load_u8(h, "brightness", &s_brightness, CHRONE_SETTINGS_BRIGHTNESS_DEFAULT);
    (void)load_u32(h, "blank_timeout_s", &s_blank_timeout_s, CHRONE_SETTINGS_BLANK_TIMEOUT_DEFAULT_S);
    (void)load_u8(h, "alarm_vol", &s_alarm_vol, CHRONE_SETTINGS_ALARM_VOL_DEFAULT);
    (void)load_u8(h, "vibe_level", &s_vibe_level, CHRONE_SETTINGS_VIBE_LEVEL_DEFAULT);
    nvs_close(h);

    s_brightness = clamp_u8(s_brightness, CHRONE_SETTINGS_BRIGHTNESS_MIN, CHRONE_SETTINGS_BRIGHTNESS_MAX);
    if (s_blank_timeout_s < 60) {
        s_blank_timeout_s = 60;
    } else if (s_blank_timeout_s > 1800) {
        s_blank_timeout_s = 1800;
    }
    s_alarm_vol = clamp_u8(s_alarm_vol, 0, 100);
    s_vibe_level = clamp_u8(s_vibe_level, 0, CHRONE_SETTINGS_VIBE_LEVEL_MAX);

    ESP_LOGI(TAG, "loaded brightness=%u blank=%lus alarm_vol=%u vibe=%u",
             s_brightness, (unsigned long)s_blank_timeout_s, s_alarm_vol, s_vibe_level);
    return ESP_OK;
}

uint8_t chrone_settings_get_brightness(void)
{
    return s_brightness;
}

esp_err_t chrone_settings_set_brightness(uint8_t percent)
{
    s_brightness = clamp_u8(percent, CHRONE_SETTINGS_BRIGHTNESS_MIN, CHRONE_SETTINGS_BRIGHTNESS_MAX);
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NS, NVS_READWRITE, &h);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = commit_u8(h, "brightness", s_brightness);
    nvs_close(h);
    return ret;
}

uint32_t chrone_settings_get_blank_timeout_s(void)
{
    return s_blank_timeout_s;
}

esp_err_t chrone_settings_set_blank_timeout_s(uint32_t seconds)
{
    if (seconds < 60) {
        seconds = 60;
    } else if (seconds > 1800) {
        seconds = 1800;
    }
    s_blank_timeout_s = seconds;
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NS, NVS_READWRITE, &h);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = commit_u32(h, "blank_timeout_s", s_blank_timeout_s);
    nvs_close(h);
    return ret;
}

uint8_t chrone_settings_get_alarm_vol(void)
{
    return s_alarm_vol;
}

esp_err_t chrone_settings_set_alarm_vol(uint8_t percent)
{
    s_alarm_vol = clamp_u8(percent, 0, 100);
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NS, NVS_READWRITE, &h);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = commit_u8(h, "alarm_vol", s_alarm_vol);
    nvs_close(h);
    return ret;
}

uint8_t chrone_settings_get_vibe_level(void)
{
    return s_vibe_level;
}

esp_err_t chrone_settings_set_vibe_level(uint8_t level)
{
    s_vibe_level = clamp_u8(level, 0, CHRONE_SETTINGS_VIBE_LEVEL_MAX);
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NS, NVS_READWRITE, &h);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = commit_u8(h, "vibe_level", s_vibe_level);
    nvs_close(h);
    return ret;
}

const char *chrone_settings_vibe_label(uint8_t level)
{
    switch (level) {
    case 0:
        return "Off";
    case 1:
        return "Weak";
    case 3:
        return "Strong";
    case 2:
    default:
        return "Med";
    }
}

