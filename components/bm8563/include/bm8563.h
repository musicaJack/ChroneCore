#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BM8563_I2C_ADDR 0x51

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t mday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t wday; /**< 0=Sunday .. 6=Saturday (BM8563 weekday register) */
} bm8563_datetime_t;

esp_err_t bm8563_init(void);
esp_err_t bm8563_get_time(bm8563_datetime_t *out);
esp_err_t bm8563_set_time(const bm8563_datetime_t *in);

#ifdef __cplusplus
}
#endif
