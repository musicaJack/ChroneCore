idf_component_register(
    SRCS "digital_clock.cpp" "analog_clock.cpp" "alarm_screen.cpp" "alarm_time_picker.cpp" "chrone_haptic.c" "chrone_alarm_icon.c" "fonts/chrone_font_dseg56.c"
    INCLUDE_DIRS "include"
    REQUIRES chrone_time chrone_wifi chrone_weather chrone_alarm chrone_input vibration espressif__m5stack_core_2 lvgl__lvgl nvs_flash esp_hw_support esp_timer
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
