#include "chrone_weather.h"

weather_info_t *chrone_weather_get_info(void)
{
    return weather_service_get_info();
}

void chrone_weather_start_query(void)
{
    weather_service_start_query();
}

void chrone_weather_get_display_city(char *buf, size_t buf_sz)
{
    weather_service_get_display_city(buf, buf_sz);
}
