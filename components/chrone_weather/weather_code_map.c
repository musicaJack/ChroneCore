/**
 * @file weather_code_map.c
 * @brief 心知天气 API 天气代码映射表实现
 */

#include "weather_code_map.h"
#include <stdio.h>
#include <string.h>

void weather_code_get_desc(int code, bool is_day, char *desc_buf, size_t buf_size)
{
    int level = 1;

    switch (code) {
        case 0:
        case 1:
        case 2:
        case 3:
            snprintf(desc_buf, buf_size, "%s", is_day ? "Sunny" : "Clear");
            return;
        case 4:
        case 5:
        case 7:
        case 8:
            snprintf(desc_buf, buf_size, "Cloudy");
            return;
        case 9:
            snprintf(desc_buf, buf_size, "Overcast");
            return;
        case 10:
        case 11:
        case 12:
            level = 1;
            snprintf(desc_buf, buf_size, "Rainy-%d", level);
            return;
        case 13:
        case 14:
        case 15:
            level = 2;
            snprintf(desc_buf, buf_size, "Rainy-%d", level);
            return;
        case 16:
        case 17:
        case 18:
            level = 3;
            snprintf(desc_buf, buf_size, "Rainy-%d", level);
            return;
        case 19:
        case 20:
        case 21:
            level = 1;
            snprintf(desc_buf, buf_size, "Snowy-%d", level);
            return;
        case 22:
        case 23:
            level = 2;
            snprintf(desc_buf, buf_size, "Snowy-%d", level);
            return;
        case 24:
        case 25:
            level = 3;
            snprintf(desc_buf, buf_size, "Snowy-%d", level);
            return;
        /* 26–37：与心知官方 code 表一致（见 docs.seniverse.com/api/start/code.html） */
        case 26:
            snprintf(desc_buf, buf_size, "Dust");
            return;
        case 27:
            snprintf(desc_buf, buf_size, "Sand");
            return;
        case 28:
            snprintf(desc_buf, buf_size, "Duststorm");
            return;
        case 29:
            snprintf(desc_buf, buf_size, "Sandstorm");
            return;
        case 30:
            snprintf(desc_buf, buf_size, "Foggy");
            return;
        case 31:
            snprintf(desc_buf, buf_size, "Haze");
            return;
        case 32:
            snprintf(desc_buf, buf_size, "Windy");
            return;
        case 33:
            snprintf(desc_buf, buf_size, "Blustery");
            return;
        case 34:
            snprintf(desc_buf, buf_size, "Hurricane");
            return;
        case 35:
            snprintf(desc_buf, buf_size, "Trop Storm");
            return;
        case 36:
            snprintf(desc_buf, buf_size, "Tornado");
            return;
        case 37:
            snprintf(desc_buf, buf_size, "Cold");
            return;
        default:
            snprintf(desc_buf, buf_size, "Unknown");
            return;
    }
}

bool weather_code_should_vibrate(int code) {
    return (code >= 10 && code <= 18) ||
           (code >= 19 && code <= 25) ||
           (code >= 31 && code <= 37);
}
