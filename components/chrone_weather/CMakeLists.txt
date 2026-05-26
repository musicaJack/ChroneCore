idf_component_register(
    SRCS
        "weather_service.c"
        "weather_city_list.c"
        "weather_code_map.c"
        "chrone_weather_api.c"
        "weather_icon_render.c"
        "weather_icons/weather_icon_map.c"
    INCLUDE_DIRS "include" "." "weather_icons"
    REQUIRES esp_http_client esp-tls json nvs_flash
)
