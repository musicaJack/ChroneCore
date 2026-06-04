idf_component_register(
    SRCS "chrone_audio.c"
    INCLUDE_DIRS "include"
    REQUIRES espressif__m5stack_core_2 chrone_settings
    EMBED_FILES "alarm_6s.pcm"
)
