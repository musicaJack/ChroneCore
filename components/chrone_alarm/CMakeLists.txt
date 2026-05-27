idf_component_register(
    SRCS "chrone_alarm.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash chrone_audio vibration chrone_time
)
