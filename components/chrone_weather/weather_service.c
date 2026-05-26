/**
 * @file weather_service.c
 * @brief 天气服务模块实现（本应用：无震动调用）
 */

#include "weather_service.h"
#include "chrone_weather_config.h"
#include "weather_code_map.h"
#include "weather_city_list.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <time.h>

static const char *TAG = "WEATHER_SERVICE";

static bool weather_is_daytime(void)
{
    time_t now;
    struct tm ti;
    time(&now);
    if (localtime_r(&now, &ti) == NULL) {
        return true;
    }
    return ti.tm_hour >= 6 && ti.tm_hour < 18;
}

/** 配网高级选项写入独立命名空间，避免与 ESP-IDF / SsidManager 共用的 "wifi" 冲突 */
#define WEATHER_NVS_NS_PRIMARY   "weather"
#define WEATHER_NVS_NS_LEGACY    "wifi"

/** 去掉 NVS 字符串首尾空白，避免误写入导致白名单不匹配 */
static void weather_trim_id_inplace(char *s)
{
    if (!s || !s[0]) {
        return;
    }
    size_t i = 0;
    while (s[i] && isspace((unsigned char)s[i])) {
        i++;
    }
    if (i > 0) {
        memmove(s, s + i, strlen(s + i) + 1);
    }
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) {
        n--;
    }
    s[n] = '\0';
}

/** 解析本次请求使用的心知 location：NVS 白名单 id 优先，否则编译默认 WEATHER_CITY */
static void weather_resolve_location_id(char *out, size_t out_sz)
{
    if (!out || out_sz < 2) {
        return;
    }
    out[0] = '\0';

    nvs_handle_t h;
    esp_err_t e;
    esp_err_t open_err;

    /* 1) 主命名空间 weather */
    open_err = nvs_open(WEATHER_NVS_NS_PRIMARY, NVS_READONLY, &h);
    if (open_err == ESP_OK) {
        size_t len = out_sz;
        e = nvs_get_str(h, WEATHER_NVS_KEY_LOCATION, out, &len);
        nvs_close(h);
        if (e == ESP_OK && out[0] != '\0') {
            weather_trim_id_inplace(out);
        }
        if (e == ESP_OK && out[0] != '\0' && weather_city_is_valid_id(out)) {
            static bool s_log_ok;
            if (!s_log_ok) {
                s_log_ok = true;
                ESP_LOGI(TAG, "[city] NVS ns=%s weather_location='%s' -> API + UI use this id",
                         WEATHER_NVS_NS_PRIMARY, out);
            }
            return;
        }
        if (e == ESP_OK && out[0] != '\0' && !weather_city_is_valid_id(out)) {
            static bool s_log_bad;
            if (!s_log_bad) {
                s_log_bad = true;
                ESP_LOGW(TAG, "[city] NVS %s has weather_location='%s' but NOT in whitelist — ignore, use fallback",
                         WEATHER_NVS_NS_PRIMARY, out);
            }
        } else if (e != ESP_OK && e != ESP_ERR_NVS_NOT_FOUND) {
            static bool s_log_get;
            if (!s_log_get) {
                s_log_get = true;
                ESP_LOGW(TAG, "[city] NVS %s nvs_get_str(%s) err=%s",
                         WEATHER_NVS_NS_PRIMARY, WEATHER_NVS_KEY_LOCATION, esp_err_to_name(e));
            }
        }
    } else {
        static bool s_log_ns;
        if (!s_log_ns) {
            s_log_ns = true;
            ESP_LOGI(TAG, "[city] NVS nvs_open('%s')=%s (no namespace or key yet — use fallback if no legacy)",
                     WEATHER_NVS_NS_PRIMARY, esp_err_to_name(open_err));
        }
    }

    out[0] = '\0';
    /* 2) 旧版写在 wifi 命名空间：读一次并迁移到 weather，避免与 WiFi 栈争用同一键 */
    open_err = nvs_open(WEATHER_NVS_NS_LEGACY, NVS_READONLY, &h);
    if (open_err == ESP_OK) {
        size_t len = out_sz;
        e = nvs_get_str(h, WEATHER_NVS_KEY_LOCATION, out, &len);
        nvs_close(h);
        if (e == ESP_OK && out[0] != '\0') {
            weather_trim_id_inplace(out);
        }
        if (e == ESP_OK && out[0] != '\0' && weather_city_is_valid_id(out)) {
            nvs_handle_t hw;
            if (nvs_open(WEATHER_NVS_NS_PRIMARY, NVS_READWRITE, &hw) == ESP_OK) {
                nvs_set_str(hw, WEATHER_NVS_KEY_LOCATION, out);
                nvs_commit(hw);
                nvs_close(hw);
                ESP_LOGI(TAG, "[city] migrated NVS %s -> %s (id='%s')",
                         WEATHER_NVS_NS_LEGACY, WEATHER_NVS_NS_PRIMARY, out);
            }
            if (nvs_open(WEATHER_NVS_NS_LEGACY, NVS_READWRITE, &h) == ESP_OK) {
                nvs_erase_key(h, WEATHER_NVS_KEY_LOCATION);
                nvs_commit(h);
                nvs_close(h);
            }
            return;
        }
    }

    {
        static bool s_log_miss;
        if (!s_log_miss) {
            s_log_miss = true;
            ESP_LOGI(TAG, "[city] no valid weather_location in NVS (ns=%s or legacy %s) -> fallback id='%s'",
                     WEATHER_NVS_NS_PRIMARY, WEATHER_NVS_NS_LEGACY, WEATHER_CITY);
        }
    }

    strncpy(out, WEATHER_CITY, out_sz - 1);
    out[out_sz - 1] = '\0';
}

static weather_info_t weather_info = {0};
#define WEATHER_REFRESH_INTERVAL_MS  (60 * 60 * 1000)  /* 1 hour */

typedef struct {
    char *buf;
    int cap;
    int len;
} resp_buf_t;

static esp_err_t weather_http_event_handler(esp_http_client_event_t *evt)
{
    if (!evt) return ESP_FAIL;
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data_len > 0 && evt->user_data) {
        resp_buf_t *rb = (resp_buf_t *)evt->user_data;
        int copy = evt->data_len;
        if (rb->len + copy >= rb->cap) {
            copy = rb->cap - rb->len - 1;
        }
        if (copy > 0) {
            memcpy(rb->buf + rb->len, evt->data, copy);
            rb->len += copy;
            rb->buf[rb->len] = '\0';
        }
    }
    return ESP_OK;
}

static void weather_task(void *pvParameters)
{
    (void)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(2000));  /* Wait for network */

    if (WEATHER_APIKEY[0] == '\0') {
        ESP_LOGW(TAG, "WEATHER_APIKEY empty — set in menuconfig, skip HTTPS");
        vTaskDelete(NULL);
        return;
    }

    for (;;) {
        char location_id[64];
        weather_resolve_location_id(location_id, sizeof(location_id));

        char url[320];
        snprintf(url, sizeof(url),
                 WEATHER_HOST "/v3/weather/now.json?key=%s&location=%s&language=en&unit=c",
                 WEATHER_APIKEY, location_id);

        ESP_LOGI(TAG, "[api] Seniverse GET now.json location_id=%s (host api.seniverse.com)", location_id);

        char resp_storage[2048];
        resp_storage[0] = '\0';
        resp_buf_t rb = {
            .buf = resp_storage,
            .cap = (int)sizeof(resp_storage),
            .len = 0,
        };

        esp_http_client_config_t config = {
            .url = url,
            .method = HTTP_METHOD_GET,
            .timeout_ms = 15000,
            .event_handler = weather_http_event_handler,
            .user_data = &rb,
            .crt_bundle_attach = esp_crt_bundle_attach,
            .buffer_size = 1024,
            .buffer_size_tx = 1024,
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client) {
            esp_err_t err = esp_http_client_perform(client);
            int status = esp_http_client_get_status_code(client);

            if (err != ESP_OK) {
                ESP_LOGE(TAG, "perform failed: %s status=%d", esp_err_to_name(err), status);
            } else if (rb.len > 0) {
                cJSON *root = cJSON_Parse(resp_storage);
                if (!root) {
                    ESP_LOGE(TAG, "JSON parse failed");
                } else {
                    const char *name = NULL;
                    const char *text = NULL;
                    const char *temperature = NULL;

                    cJSON *results = cJSON_GetObjectItem(root, "results");
                    cJSON *r0 = (cJSON_IsArray(results) ? cJSON_GetArrayItem(results, 0) : NULL);
                    cJSON *location = (r0 ? cJSON_GetObjectItem(r0, "location") : NULL);
                    cJSON *now = (r0 ? cJSON_GetObjectItem(r0, "now") : NULL);

                    cJSON *jname = (location ? cJSON_GetObjectItem(location, "name") : NULL);
                    cJSON *jtext = (now ? cJSON_GetObjectItem(now, "text") : NULL);
                    cJSON *jcode = (now ? cJSON_GetObjectItem(now, "code") : NULL);
                    cJSON *jtemp = (now ? cJSON_GetObjectItem(now, "temperature") : NULL);

                    if (cJSON_IsString(jname)) name = jname->valuestring;
                    if (cJSON_IsString(jtext)) text = jtext->valuestring;
                    if (cJSON_IsString(jtemp)) temperature = jtemp->valuestring;

                    int parsed_code = -1;
                    if (jcode) {
                        if (cJSON_IsString(jcode)) {
                            parsed_code = atoi(jcode->valuestring);
                        } else if (cJSON_IsNumber(jcode)) {
                            parsed_code = jcode->valueint;
                        }
                    }

                    if (name && temperature) {
                        /* UI：按所选 location id 显示首字母大写拼音（如 Shanghai、Jiangsu Taizhou） */
                        weather_city_format_title_case(location_id, weather_info.city, sizeof(weather_info.city));
                        weather_info.code = parsed_code;

                        char weather_desc_buf[32];
                        if (weather_info.code >= 0) {
                            weather_code_get_desc(weather_info.code, weather_is_daytime(),
                                                  weather_desc_buf, sizeof(weather_desc_buf));
                        } else if (text) {
                            strncpy(weather_desc_buf, text, sizeof(weather_desc_buf) - 1);
                            weather_desc_buf[sizeof(weather_desc_buf) - 1] = '\0';
                        } else {
                            strncpy(weather_desc_buf, "Unknown", sizeof(weather_desc_buf) - 1);
                        }
                        strncpy(weather_info.text, weather_desc_buf, sizeof(weather_info.text) - 1);
                        weather_info.text[sizeof(weather_info.text) - 1] = '\0';
                        strncpy(weather_info.temp, temperature, sizeof(weather_info.temp) - 1);
                        weather_info.temp[sizeof(weather_info.temp) - 1] = '\0';
                        weather_info.available = true;
                        ESP_LOGI(TAG, "[api] parsed OK: display_city='%s' temp=%s°C code=%d (title from id=%s)",
                                 weather_info.city, weather_info.temp, weather_info.code, location_id);
                    }
                    cJSON_Delete(root);
                }
            }

            esp_http_client_cleanup(client);
        } else {
            ESP_LOGE(TAG, "esp_http_client_init failed");
        }

        vTaskDelay(pdMS_TO_TICKS(WEATHER_REFRESH_INTERVAL_MS));
    }
}

static bool s_weather_task_started = false;

void weather_service_start_query(void)
{
    if (s_weather_task_started) {
        return;
    }
    s_weather_task_started = true;
    ESP_LOGI(TAG, "weather_service_start_query: task created (first HTTP ~2s after boot, then hourly)");
    xTaskCreate(weather_task, "weather_task", 8192, NULL, 5, NULL);
}

weather_info_t* weather_service_get_info(void)
{
    return weather_info.available ? &weather_info : NULL;
}

bool weather_service_should_vibrate(int code)
{
    return weather_code_should_vibrate(code);
}

void weather_service_get_display_city(char *buf, size_t buf_sz)
{
    if (!buf || buf_sz == 0) {
        return;
    }
    buf[0] = '\0';
    char loc[64];
    weather_resolve_location_id(loc, sizeof(loc));
    weather_city_format_title_case(loc, buf, buf_sz);
}
