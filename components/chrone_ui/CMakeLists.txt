idf_component_register(
    SRCS "digital_clock.cpp" "analog_clock.cpp" "fonts/chrone_font_dseg56.c"
    INCLUDE_DIRS "include"
    REQUIRES chrone_time chrone_wifi chrone_weather lvgl__lvgl nvs_flash esp_hw_support
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
