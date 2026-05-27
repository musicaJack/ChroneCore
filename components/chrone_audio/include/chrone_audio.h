#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t chrone_audio_init(void);
void chrone_audio_alarm_start(void);
void chrone_audio_alarm_stop(void);

#ifdef __cplusplus
}
#endif
