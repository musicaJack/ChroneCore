#pragma once

#include "sdkconfig.h"

#define WEATHER_HOST "https://api.seniverse.com"

#ifndef WEATHER_APIKEY
#define WEATHER_APIKEY CONFIG_CHRONECORE_WEATHER_API_KEY
#endif

#ifndef WEATHER_CITY
#define WEATHER_CITY CONFIG_CHRONECORE_WEATHER_DEFAULT_CITY
#endif
