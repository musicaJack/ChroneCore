idf_component_register(
    SRCS "main.cpp"
    REQUIRES nvs_flash chrone_hal chrone_time chrone_app
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
