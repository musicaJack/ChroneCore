idf_component_register(
    SRCS "chrone_time.c"
    INCLUDE_DIRS "include"
    REQUIRES bm8563 lwip esp_netif
)
