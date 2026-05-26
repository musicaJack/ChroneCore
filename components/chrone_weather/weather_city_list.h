/**
 * @file weather_city_list.h
 * @brief 心知 weather API 的 location 与城市中文名（唯一列表源，供配网页 JSON + NVS 校验 + 天气请求）
 *
 * 详见 docs/WEATHER_CITY_CONFIG_SOLUTION.md
 */

#ifndef WEATHER_CITY_LIST_H
#define WEATHER_CITY_LIST_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * NVS 中存储心知 location id 的键名。
 * ESP-IDF 要求键名长度 ≤15 字符；勿使用 "weather_location"（16 字符，会返回 ESP_ERR_NVS_KEY_TOO_LONG）。
 * 配网页 JSON 仍使用字段名 weather_location，仅 NVS 键用此宏。
 */
#define WEATHER_NVS_KEY_LOCATION "wx_city"

typedef struct {
    const char *id;       /**< 心知 location，小写拼音或「省拼音 城市拼音」 */
    const char *name_zh;  /**< 配网页展示用中文 */
} weather_city_entry_t;

/** 数组元素个数 */
size_t weather_city_get_count(void);

/** 按下标取条目，index 超界返回 NULL */
const weather_city_entry_t *weather_city_get_entry(size_t index);

/**
 * @brief 判断 id 是否在白名单（与 kWeatherCities[].id 精确匹配）
 */
bool weather_city_is_valid_id(const char *id);

/**
 * @brief 按 location id 取中文展示名；不在白名单则返回 NULL
 */
const char *weather_city_get_name_zh(const char *id);

/**
 * @brief 将心知 location id（小写拼音，可含空格分词）格式化为界面用「首字母大写」拉丁显示，如 shanghai→Shanghai，jiangsu taizhou→Jiangsu Taizhou
 */
void weather_city_format_title_case(const char *id, char *out, size_t out_sz);

#ifdef __cplusplus
}
#endif

#endif /* WEATHER_CITY_LIST_H */
