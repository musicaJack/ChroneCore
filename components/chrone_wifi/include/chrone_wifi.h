#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CHRONE_WIFI_STATE_IDLE = 0,
    CHRONE_WIFI_STATE_CONNECTING,
    CHRONE_WIFI_STATE_CONNECTED,
    CHRONE_WIFI_STATE_PROVISIONING,
} chrone_wifi_state_t;

/** 后台启动：STA 连接或进入 AP 配网（AP 时阻塞直到重启）。 */
esp_err_t chrone_wifi_start(void);

chrone_wifi_state_t chrone_wifi_get_state(void);
bool chrone_wifi_is_connected(void);

/** 配网模式下填充 SSID 与 Web URL（http://192.168.4.1） */
esp_err_t chrone_wifi_get_provisioning_hint(char *ssid_buf, size_t ssid_sz, char *url_buf, size_t url_sz);

/** 设置 force_ap 并重启，进入配网 */
void chrone_wifi_reset_configuration(void);

#ifdef __cplusplus
}
#endif
