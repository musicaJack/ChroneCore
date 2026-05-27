#include "chrone_alarm.h"

#include "chrone_audio.h"
#include "vibration/vibration.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "chrone_alarm";
static const char *NVS_NS = "chrone";
static const char *NVS_KEY = "alarm_cfg";

#define CFG_MAGIC 0x414C524Du /* "ALRM" */
#define CFG_VERSION 1

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t version;
    uint8_t reserved[3];
} chrone_alarm_cfg_hdr_t;

typedef struct __attribute__((packed)) {
    chrone_alarm_cfg_hdr_t hdr;
    chrone_alarm_t alarms[CHRONE_ALARM_MAX];
} chrone_alarm_cfg_blob_t;

static chrone_alarm_t s_alarms[CHRONE_ALARM_MAX];
static bool s_nvs_ok;
static chrone_alarm_state_t s_state = CHRONE_ALARM_STATE_IDLE;
static int s_ringing_index = -1;
static uint32_t s_ring_start_ms;
static uint32_t s_last_vibe_ms;
static int s_last_fire_ymd = -1;
static int s_last_fire_hm = -1;
static int s_last_mday_seen = -1;

#define RING_TIMEOUT_MS 60000
#define VIBE_INTERVAL_MS 700

static bool is_weekday(int tm_wday)
{
    return tm_wday >= 1 && tm_wday <= 5;
}

static void clear_once_fired_flags_if_new_day(int mday)
{
    if (s_last_mday_seen == mday) {
        return;
    }
    s_last_mday_seen = mday;
    for (int i = 0; i < CHRONE_ALARM_MAX; ++i) {
        if (s_alarms[i].repeat == CHRONE_ALARM_REPEAT_ONCE) {
            s_alarms[i].flags &= (uint8_t)~CHRONE_ALARM_FLAG_FIRED_TODAY;
        }
    }
}

static bool blob_valid(const chrone_alarm_cfg_blob_t *blob)
{
    return blob && blob->hdr.magic == CFG_MAGIC && blob->hdr.version == CFG_VERSION;
}

static void defaults(void)
{
    memset(s_alarms, 0, sizeof(s_alarms));
}

static esp_err_t read_blob(chrone_alarm_cfg_blob_t *blob)
{
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (ret != ESP_OK) {
        return ret;
    }

    size_t len = sizeof(*blob);
    ret = nvs_get_blob(h, NVS_KEY, blob, &len);
    nvs_close(h);
    if (ret != ESP_OK) {
        return ret;
    }
    if (len != sizeof(*blob) || !blob_valid(blob)) {
        return ESP_ERR_INVALID_VERSION;
    }
    return ESP_OK;
}

static esp_err_t write_blob(void)
{
    chrone_alarm_cfg_blob_t blob = {
        .hdr = {
            .magic = CFG_MAGIC,
            .version = CFG_VERSION,
        },
    };
    memcpy(blob.alarms, s_alarms, sizeof(s_alarms));

    nvs_handle_t h;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_set_blob(h, NVS_KEY, &blob, sizeof(blob));
    if (ret == ESP_OK) {
        ret = nvs_commit(h);
    }
    nvs_close(h);
    return ret;
}

static void log_alarms(void)
{
    for (int i = 0; i < CHRONE_ALARM_MAX; ++i) {
        const chrone_alarm_t *a = &s_alarms[i];
        ESP_LOGI(TAG, "[%d] %s %02u:%02u repeat=%u flags=0x%02x",
                 i, a->enabled ? "ON" : "off", a->hour, a->minute, a->repeat, a->flags);
    }
}

static bool repeat_matches(const chrone_alarm_t *a, const struct tm *now)
{
    switch (a->repeat) {
    case CHRONE_ALARM_REPEAT_ONCE:
        return !(a->flags & CHRONE_ALARM_FLAG_FIRED_TODAY);
    case CHRONE_ALARM_REPEAT_DAILY:
        return true;
    case CHRONE_ALARM_REPEAT_WEEKDAY:
        return is_weekday(now->tm_wday);
    default:
        return false;
    }
}

static void mark_once_fired(chrone_alarm_t *a)
{
    a->flags |= CHRONE_ALARM_FLAG_FIRED_TODAY;
    if (a->repeat == CHRONE_ALARM_REPEAT_ONCE) {
        a->enabled = false;
    }
}

static void start_ringing(int index)
{
    if (s_state == CHRONE_ALARM_STATE_RINGING) {
        return;
    }
    s_state = CHRONE_ALARM_STATE_RINGING;
    s_ringing_index = index;
    s_ring_start_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    s_last_vibe_ms = 0;
    chrone_audio_alarm_start();
    (void)vibration_trigger();
    ESP_LOGW(TAG, "RINGING alarm[%d] %02u:%02u", index,
             s_alarms[index].hour, s_alarms[index].minute);
}

void chrone_alarm_dismiss(void)
{
    if (s_state != CHRONE_ALARM_STATE_RINGING) {
        return;
    }
    chrone_audio_alarm_stop();
    (void)vibration_stop();
    s_state = CHRONE_ALARM_STATE_IDLE;
    s_ringing_index = -1;
    ESP_LOGI(TAG, "alarm dismissed");
    (void)chrone_alarm_save();
}

esp_err_t chrone_alarm_load(void)
{
    defaults();
    s_nvs_ok = false;

    chrone_alarm_cfg_blob_t blob;
    esp_err_t ret = read_blob(&blob);
    if (ret == ESP_OK) {
        memcpy(s_alarms, blob.alarms, sizeof(s_alarms));
        s_nvs_ok = true;
        ESP_LOGI(TAG, "loaded from NVS");
    } else {
        ESP_LOGI(TAG, "no valid alarm_cfg (%s), scheduling off", esp_err_to_name(ret));
    }

    log_alarms();
    return ret;
}

esp_err_t chrone_alarm_save(void)
{
    esp_err_t ret = write_blob();
    if (ret == ESP_OK) {
        s_nvs_ok = true;
        ESP_LOGI(TAG, "saved alarm_cfg");
        log_alarms();
    } else {
        ESP_LOGE(TAG, "save failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t chrone_alarm_get(uint8_t index, chrone_alarm_t *out)
{
    if (!out || index >= CHRONE_ALARM_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    *out = s_alarms[index];
    return ESP_OK;
}

esp_err_t chrone_alarm_set(uint8_t index, const chrone_alarm_t *alarm)
{
    if (!alarm || index >= CHRONE_ALARM_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    if (alarm->hour > 23 || alarm->minute > 59 || alarm->repeat > CHRONE_ALARM_REPEAT_WEEKDAY) {
        return ESP_ERR_INVALID_ARG;
    }
    s_alarms[index] = *alarm;
    return ESP_OK;
}

bool chrone_alarm_scheduling_enabled(void)
{
    if (!s_nvs_ok) {
        return false;
    }
    for (int i = 0; i < CHRONE_ALARM_MAX; ++i) {
        if (s_alarms[i].enabled) {
            return true;
        }
    }
    return false;
}

chrone_alarm_state_t chrone_alarm_get_state(void)
{
    return s_state;
}

int chrone_alarm_ringing_index(void)
{
    return s_ringing_index;
}

void chrone_alarm_check_tick(const struct tm *now)
{
    if (!now) {
        return;
    }

    clear_once_fired_flags_if_new_day(now->tm_mday);

    if (s_state == CHRONE_ALARM_STATE_RINGING) {
        return;
    }

    if (!chrone_alarm_scheduling_enabled()) {
        return;
    }

    const int ymd = (now->tm_year + 1900) * 10000 + (now->tm_mon + 1) * 100 + now->tm_mday;
    const int hm = now->tm_hour * 60 + now->tm_min;
    if (ymd == s_last_fire_ymd && hm == s_last_fire_hm) {
        return;
    }

    for (int i = 0; i < CHRONE_ALARM_MAX; ++i) {
        chrone_alarm_t *a = &s_alarms[i];
        if (!a->enabled) {
            continue;
        }
        if (a->hour != (uint8_t)now->tm_hour || a->minute != (uint8_t)now->tm_min) {
            continue;
        }
        if (!repeat_matches(a, now)) {
            continue;
        }

        mark_once_fired(a);
        s_last_fire_ymd = ymd;
        s_last_fire_hm = hm;
        start_ringing(i);
        return;
    }
}

void chrone_alarm_poll(void)
{
    if (s_state != CHRONE_ALARM_STATE_RINGING) {
        return;
    }
    const uint32_t now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

    if (s_last_vibe_ms == 0 || (now_ms - s_last_vibe_ms) >= VIBE_INTERVAL_MS) {
        s_last_vibe_ms = now_ms;
        (void)vibration_trigger();
    }

    if (now_ms - s_ring_start_ms >= RING_TIMEOUT_MS) {
        ESP_LOGI(TAG, "ring timeout");
        chrone_alarm_dismiss();
    }
}
