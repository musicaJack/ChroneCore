idf_component_register(
    SRCS "chrone_wifi.cpp" "chrone_settings.cpp"
    INCLUDE_DIRS "include"
    REQUIRES chrone_esp_wifi_connect chrone_time chrone_weather esp_wifi nvs_flash
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
