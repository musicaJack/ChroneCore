set(esp_wifi_connect_srcs
    "wifi_configuration_ap.cc"
    "wifi_station.cc"
    "ssid_manager.cc"
    "dns_server.cc"
)

idf_component_register(
    SRCS ${esp_wifi_connect_srcs}
    INCLUDE_DIRS "include"
    EMBED_TXTFILES
        "assets/wifi_configuration.html"
        "assets/wifi_configuration_done.html"
    REQUIRES
        esp_timer
        esp_http_server
        esp_wifi
        nvs_flash
        json
        chrone_weather
)
