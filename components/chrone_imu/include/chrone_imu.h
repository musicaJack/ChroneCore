#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t chrone_imu_init(void);
bool chrone_imu_shake_detected(void);

#ifdef __cplusplus
}
#endif
