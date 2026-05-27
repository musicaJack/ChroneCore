#include "chrone_alarm.h"

#include "chrone_audio.h"
#include "chrone_time.h"
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

#define RING_TIMEOUT_MS 60000
#define VIBE_INTERVAL_MS 700

/** 旧 NVS：1=Daily → 现 0=Daily；0/1/2=Daily/Workday/Once */
static uint8_t migrate_repeat(uint8_t repeat)
{
    if (repeat == 1) {
        return CHRONE_ALARM_REPEAT_DAILY;
    }
    if (repeat > CHRONE_ALARM_REPEAT_ONCE) {
        return CHRONE_ALARM_REPEAT_DAILY;
    }
    return repeat;
}

static bool is_workday(int tm_wday)
{
    return tm_wday >= 1 && tm_wday <= 5;
}

static bool normalize_alarms(void)
{
    bool changed = false;
    for (int i = 0; i < CHRONE_ALARM_MAX; ++i) {
        chrone_alarm_t *a = &s_alarms[i];
        const uint8_t old_repeat = a->repeat;
        const uint8_t old_flags = a->flags;
        a->repeat = migrate_repeat(a->repeat);
        a->flags = 0;
        if (old_repeat != a->repeat || old_flags != 0) {
            changed = true;
            ESP_LOGI(TAG, "migrate alarm[%d] repeat %u -> %u", i, old_repeat, a->repeat);
        }
    }
    return changed;
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
        ESP_LOGI(TAG, "[%d] %s %02u:%02u repeat=%u",
                 i, a->enabled ? "ON" : "off", a->hour, a->minute, a->repeat);
    }
}

static const char *repeat_name(uint8_t repeat)
{
    switch (repeat) {
    case CHRONE_ALARM_REPEAT_WORKDAY:
        return "Workday";
    case CHRONE_ALARM_REPEAT_ONCE:
        return "Once";
    case CHRONE_ALARM_REPEAT_DAILY:
    default:
        return "Daily";
    }
}

/** 每分钟 :00 打一条，确认 1 Hz tick 与时间源正常 */
static void log_heartbeat(const struct tm *now)
{
    if (now->tm_sec != 0) {
        return;
    }
    ESP_LOGI(TAG,
             "tick %04d-%02d-%02d %02d:%02d:%02d wday=%d nvs_ok=%d sched=%d state=%d "
             "last_fire_ymd=%d last_fire_hm=%d",
             now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
             now->tm_hour, now->tm_min, now->tm_sec, now->tm_wday,
             s_nvs_ok, chrone_alarm_scheduling_enabled(), (int)s_state,
             s_last_fire_ymd, s_last_fire_hm);
    for (int i = 0; i < CHRONE_ALARM_MAX; ++i) {
        const chrone_alarm_t *a = &s_alarms[i];
        if (!a->enabled) {
            continue;
        }
        ESP_LOGI(TAG, "  armed[%d] %02u:%02u %s",
                 i, a->hour, a->minute, repeat_name(a->repeat));
    }
}

static void log_slot_skip(int index, const chrone_alarm_t *a, const struct tm *now, const char *reason)
{
    ESP_LOGW(TAG,
             "skip alarm[%d] %02u:%02u at %02d:%02d:%02d: %s (repeat=%s wday=%d)",
             index, a->hour, a->minute, now->tm_hour, now->tm_min, now->tm_sec, reason,
             repeat_name(a->repeat), now->tm_wday);
}

static bool repeat_matches(const chrone_alarm_t *a, const struct tm *now)
{
    switch (a->repeat) {
    case CHRONE_ALARM_REPEAT_DAILY:
    case CHRONE_ALARM_REPEAT_ONCE:
        return true;
    case CHRONE_ALARM_REPEAT_WORKDAY:
        return is_workday(now->tm_wday);
    default:
        return false;
    }
}

bool chrone_alarm_once_time_ok(uint8_t hour, uint8_t minute)
{
    struct tm now;
    if (chrone_time_get_local_tm(&now) != ESP_OK) {
        return false;
    }
    const int now_sec = now.tm_hour * 3600 + now.tm_min * 60 + now.tm_sec;
    const int alarm_sec = (int)hour * 3600 + (int)minute * 60;
    int delta_sec;
    if (alarm_sec > now_sec) {
        delta_sec = alarm_sec - now_sec;
    } else {
        delta_sec = 24 * 3600 - now_sec + alarm_sec;
    }
    return delta_sec >= (CHRONE_ALARM_ONCE_MIN_LEAD_MIN * 60);
}

static void start_ringing(int index)
{
    if (s_state == CHRONE_ALARM_STATE_RINGING) {
        ESP_LOGW(TAG, "start_ringing[%d] ignored, already ringing[%d]", index, s_ringing_index);
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
        if (normalize_alarms()) {
            (void)chrone_alarm_save();
        }
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
    if (alarm->hour > 23 || alarm->minute > 59
        || alarm->repeat > CHRONE_ALARM_REPEAT_ONCE) {
        return ESP_ERR_INVALID_ARG;
    }
    if (alarm->enabled && alarm->repeat == CHRONE_ALARM_REPEAT_ONCE
        && !chrone_alarm_once_time_ok(alarm->hour, alarm->minute)) {
        return ESP_ERR_INVALID_ARG;
    }
    s_alarms[index] = *alarm;
    s_alarms[index].flags = 0;
    return ESP_OK;
}

bool chrone_alarm_scheduling_enabled(void)
{
    return chrone_alarm_enabled_count() > 0;
}

uint8_t chrone_alarm_enabled_count(void)
{
    if (!s_nvs_ok) {
        return 0;
    }
    uint8_t n = 0;
    for (int i = 0; i < CHRONE_ALARM_MAX; ++i) {
        if (s_alarms[i].enabled) {
            ++n;
        }
    }
    return n;
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

    log_heartbeat(now);

    if (s_state == CHRONE_ALARM_STATE_RINGING) {
        if (now->tm_sec == 0) {
            ESP_LOGD(TAG, "check_tick: busy ringing[%d]", s_ringing_index);
        }
        return;
    }

    if (!chrone_alarm_scheduling_enabled()) {
        if (now->tm_sec == 0) {
            ESP_LOGW(TAG, "check_tick: scheduling off (nvs_ok=%d)", s_nvs_ok);
        }
        return;
    }

    const int ymd = (now->tm_year + 1900) * 10000 + (now->tm_mon + 1) * 100 + now->tm_mday;
    const int hm = now->tm_hour * 60 + now->tm_min;
    if (ymd == s_last_fire_ymd && hm == s_last_fire_hm) {
        if (now->tm_sec == 0) {
            for (int i = 0; i < CHRONE_ALARM_MAX; ++i) {
                const chrone_alarm_t *a = &s_alarms[i];
                if (a->enabled && a->hour == (uint8_t)now->tm_hour
                    && a->minute == (uint8_t)now->tm_min) {
                    log_slot_skip(i, a, now, "dedup same minute (already fired this hm)");
                    break;
                }
            }
        }
        return;
    }

    bool any_hm_match = false;
    for (int i = 0; i < CHRONE_ALARM_MAX; ++i) {
        chrone_alarm_t *a = &s_alarms[i];
        if (!a->enabled) {
            continue;
        }
        if (a->hour != (uint8_t)now->tm_hour || a->minute != (uint8_t)now->tm_min) {
            continue;
        }
        any_hm_match = true;
        if (!repeat_matches(a, now)) {
            if (a->repeat == CHRONE_ALARM_REPEAT_WORKDAY) {
                log_slot_skip(i, a, now, "Workday but today is weekend");
            } else {
                log_slot_skip(i, a, now, "repeat rule mismatch");
            }
            continue;
        }

        ESP_LOGI(TAG, "FIRE alarm[%d] %02u:%02u at %04d-%02d-%02d %02d:%02d:%02d",
                 i, a->hour, a->minute,
                 now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
                 now->tm_hour, now->tm_min, now->tm_sec);
        if (a->repeat == CHRONE_ALARM_REPEAT_ONCE) {
            a->enabled = false;
            (void)chrone_alarm_save();
        }
        s_last_fire_ymd = ymd;
        s_last_fire_hm = hm;
        start_ringing(i);
        return;
    }

    if (any_hm_match) {
        ESP_LOGW(TAG, "hm match at %02d:%02d:%02d but no slot fired (see skip above)",
                 now->tm_hour, now->tm_min, now->tm_sec);
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
