#include "chrone_time.h"

#include "bm8563.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include <string.h>
#include <sys/time.h>

static const char *TAG = "chrone_time";

static bool s_rtc_synced;
static bool s_sntp_synced;

static void bm8563_to_tm(const bm8563_datetime_t *rtc, struct tm *out)
{
    memset(out, 0, sizeof(*out));
    out->tm_year = (int)rtc->year - 1900;
    out->tm_mon = (int)rtc->month - 1;
    out->tm_mday = rtc->mday;
    out->tm_hour = rtc->hour;
    out->tm_min = rtc->minute;
    out->tm_sec = rtc->second;
    out->tm_wday = rtc->wday;
}

static void tm_to_bm8563(const struct tm *in, bm8563_datetime_t *rtc)
{
    rtc->year = (uint16_t)(in->tm_year + 1900);
    rtc->month = (uint8_t)(in->tm_mon + 1);
    rtc->mday = (uint8_t)in->tm_mday;
    rtc->hour = (uint8_t)in->tm_hour;
    rtc->minute = (uint8_t)in->tm_min;
    rtc->second = (uint8_t)in->tm_sec;
    rtc->wday = (uint8_t)in->tm_wday;
}

static esp_err_t apply_system_time(const struct tm *tm_local)
{
    struct tm copy = *tm_local;
    copy.tm_isdst = -1;
    time_t t = mktime(&copy);
    if (t == (time_t)-1) {
        ESP_LOGE(TAG, "mktime failed");
        return ESP_FAIL;
    }
    struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
    if (settimeofday(&tv, NULL) != 0) {
        ESP_LOGE(TAG, "settimeofday failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void sntp_sync_cb(struct timeval *tv)
{
    (void)tv;
    s_sntp_synced = true;
    ESP_LOGI(TAG, "SNTP time synced");

    struct tm tm_local;
    if (chrone_time_get_local_tm(&tm_local) == ESP_OK) {
        bm8563_datetime_t rtc;
        tm_to_bm8563(&tm_local, &rtc);
        if (bm8563_set_time(&rtc) == ESP_OK) {
            ESP_LOGI(TAG, "BM8563 updated from SNTP");
        }
    }
}

const char *chrone_time_weekday_zh(int tm_wday)
{
    static const char *names[] = {
        "星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六",
    };
    if (tm_wday < 0 || tm_wday > 6) {
        return "---";
    }
    return names[tm_wday];
}

const char *chrone_time_weekday_en(int tm_wday)
{
    static const char *names[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday",
    };
    if (tm_wday < 0 || tm_wday > 6) {
        return "---";
    }
    return names[tm_wday];
}

esp_err_t chrone_time_init(void)
{
    esp_err_t ret = bm8563_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "bm8563_init failed");
        return ret;
    }

    bm8563_datetime_t rtc;
    ret = bm8563_get_time(&rtc);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "bm8563_get_time failed");
        return ret;
    }

    struct tm tm_local;
    bm8563_to_tm(&rtc, &tm_local);
    ret = apply_system_time(&tm_local);
    if (ret != ESP_OK) {
        return ret;
    }

    s_rtc_synced = true;
    ESP_LOGI(TAG, "RTC -> system: %04d-%02d-%02d %02d:%02d:%02d %s",
             rtc.year, rtc.month, rtc.mday, rtc.hour, rtc.minute, rtc.second,
             chrone_time_weekday_zh(tm_local.tm_wday));
    return ESP_OK;
}

esp_err_t chrone_time_get_local_tm(struct tm *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    time_t now = time(NULL);
    if (localtime_r(&now, out) == NULL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t chrone_time_set_local_tm(const struct tm *in)
{
    if (!in) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = apply_system_time(in);
    if (ret != ESP_OK) {
        return ret;
    }
    bm8563_datetime_t rtc;
    tm_to_bm8563(in, &rtc);
    return bm8563_set_time(&rtc);
}

bool chrone_time_rtc_synced(void)
{
    return s_rtc_synced;
}

esp_err_t chrone_time_sync_sntp_start(void)
{
    if (esp_sntp_enabled()) {
        return ESP_OK;
    }
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.aliyun.com");
    esp_sntp_setservername(1, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb(sntp_sync_cb);
    esp_sntp_init();
    ESP_LOGI(TAG, "SNTP started (waiting for network)");
    return ESP_OK;
}

bool chrone_time_sntp_synced(void)
{
    return s_sntp_synced;
}
