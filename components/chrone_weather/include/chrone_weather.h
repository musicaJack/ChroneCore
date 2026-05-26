#pragma once

#include "weather_service.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 与 weather_info_t 相同，供 UI 使用 */
typedef weather_info_t chrone_weather_info_t;

weather_info_t *chrone_weather_get_info(void);
void chrone_weather_start_query(void);
void chrone_weather_get_display_city(char *buf, size_t buf_sz);

#ifdef __cplusplus
}
#endif
