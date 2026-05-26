idf_component_register(
    SRCS "app_controller.cpp" "clock_screen.cpp"
    INCLUDE_DIRS "include"
    REQUIRES chrone_hal chrone_time chrone_ui chrone_wifi espressif__m5stack_core_2
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
