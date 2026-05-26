/**
 * @file weather_icon_map.c
 * @brief 心知天气代码到天气图标的映射实现
 */

#include "weather_icon_map.h"
#include <time.h>

// 包含所有需要的图标头文件
#include "icons_48x48/wi_day_sunny_48x48.h"
#include "icons_48x48/wi_day_cloudy_48x48.h"
#include "icons_48x48/wi_cloudy_48x48.h"
#include "icons_48x48/wi_day_rain_48x48.h"
#include "icons_48x48/wi_rain_48x48.h"
#include "icons_48x48/wi_showers_48x48.h"
#include "icons_48x48/wi_day_snow_48x48.h"
#include "icons_48x48/wi_snow_48x48.h"
#include "icons_48x48/wi_day_fog_48x48.h"
#include "icons_48x48/wi_fog_48x48.h"
#include "icons_48x48/wi_thunderstorm_48x48.h"
#include "icons_48x48/wi_day_thunderstorm_48x48.h"
#include "icons_48x48/wi_tornado_48x48.h"
#include "icons_48x48/wi_na_48x48.h"
#include "icons_48x48/wi_day_sleet_48x48.h"
#include "icons_48x48/wi_sleet_48x48.h"
#include "icons_48x48/wi_sandstorm_48x48.h"
#include "icons_48x48/wi_dust_48x48.h"
#include "icons_48x48/wi_hail_48x48.h"
#include "icons_48x48/wi_day_hail_48x48.h"
#include "icons_48x48/wi_strong_wind_48x48.h"
#include "icons_48x48/wi_day_windy_48x48.h"
#include "icons_48x48/wi_cloudy_gusts_48x48.h"
#include "icons_48x48/wi_day_cloudy_gusts_48x48.h"
#include "icons_48x48/wi_moon_48x48.h"
#include "icons_48x48/wi_night_clear_48x48.h"
#include "icons_48x48/wi_night_alt_cloudy_48x48.h"
#include "icons_48x48/wi_night_alt_rain_48x48.h"
#include "icons_48x48/wi_night_alt_snow_48x48.h"
#include "icons_48x48/wi_night_fog_48x48.h"
#include "icons_48x48/wi_night_alt_thunderstorm_48x48.h"

/**
 * @brief 判断当前是否为白天
 * 
 * @return true 白天 (6:00-18:00)
 * @return false 夜晚
 */
static bool is_daytime(void)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    int hour = timeinfo.tm_hour;
    return (hour >= 6 && hour < 18);
}

const uint8_t* weather_icon_get_bitmap(int code, bool is_day)
{
    // is_day 参数由调用者根据当前时间传入
    // 如果调用者传入 false，这里会自动判断是否为白天
    // 但通常调用者应该已经判断好了，这里保留自动判断作为备用
    if (!is_day) {
        is_day = is_daytime();
    }
    
    switch (code) {
        // 0-3: 晴
        case 0:
        case 1:
        case 2:
        case 3:
            return is_day ? wi_day_sunny_48x48 : wi_moon_48x48;
        
        // 4-5, 7-8: 多云
        case 4:
        case 5:
        case 7:
        case 8:
            return is_day ? wi_day_cloudy_48x48 : wi_night_alt_cloudy_48x48;
        
        // 9: 阴
        case 9:
            return wi_cloudy_48x48;
        
        // 10-12: 小雨
        case 10:
        case 11:
        case 12:
            return is_day ? wi_day_rain_48x48 : wi_night_alt_rain_48x48;
        
        // 13-15: 中雨
        case 13:
        case 14:
        case 15:
            return is_day ? wi_rain_48x48 : wi_night_alt_rain_48x48;
        
        // 16-18: 大雨/暴雨/雷阵雨
        case 16:
        case 17:
        case 18:
            return is_day ? wi_day_thunderstorm_48x48 : wi_night_alt_thunderstorm_48x48;
        
        // 19-21: 小雪
        case 19:
        case 20:
        case 21:
            return is_day ? wi_day_snow_48x48 : wi_night_alt_snow_48x48;
        
        // 22-23: 中雪
        case 22:
        case 23:
            return is_day ? wi_snow_48x48 : wi_night_alt_snow_48x48;
        
        // 24-25: 大雪/暴雪
        case 24:
        case 25:
            return is_day ? wi_snow_48x48 : wi_night_alt_snow_48x48;
        
        // 26-28: 雾
        case 26:
        case 27:
        case 28:
            return is_day ? wi_day_fog_48x48 : wi_night_fog_48x48;
        
        // 29-30: 沙尘/扬沙/浮尘
        case 29:
        case 30:
            return wi_sandstorm_48x48;
        
        // 31-33: 大风等恶劣天气
        case 31:
        case 32:
        case 33:
            return is_day ? wi_day_windy_48x48 : wi_strong_wind_48x48;
        
        // 34-35: 飓风等
        case 34:
        case 35:
            return wi_tornado_48x48;
        
        // 36-37: 冰雹等
        case 36:
        case 37:
            return is_day ? wi_day_hail_48x48 : wi_hail_48x48;
        
        // 未知代码
        default:
            return wi_na_48x48;
    }
}
