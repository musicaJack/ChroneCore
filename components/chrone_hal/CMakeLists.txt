idf_component_register(
    SRCS "chrone_hal.c" "chrone_hal_touch.c"
    INCLUDE_DIRS "include"
    REQUIRES driver axp192_esp32 espressif__m5stack_core_2 espressif__esp_lvgl_port
    PRIV_REQUIRES espressif__esp_lcd_touch_ft5x06
)
