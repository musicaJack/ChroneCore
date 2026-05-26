/**
 * @file weather_service.h
 * @brief 天气服务模块 - 提供天气查询、数据解析（本应用，无震动）
 */

#ifndef WEATHER_SERVICE_H
#define WEATHER_SERVICE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char city[32];
    char text[32];
    char temp[8];
    int code;
    bool available;
} weather_info_t;

/** 获取当前天气信息，不可用则返回 NULL */
weather_info_t* weather_service_get_info(void);

/** 启动异步天气查询任务（需在 WiFi 已连接后调用） */
void weather_service_start_query(void);

/**
 * @brief 按 NVS 中 weather_location（及白名单）解析用于界面显示的中文城市名；
 *        无有效配置时使用默认 WEATHER_CITY 对应中文或拼音。
 * @param buf 输出缓冲
 * @param buf_sz sizeof(buf)
 */
void weather_service_get_display_city(char *buf, size_t buf_sz);

/** 检查天气代码是否需要震动（本设备无马达，仅保留接口） */
bool weather_service_should_vibrate(int code);

#ifdef __cplusplus
}
#endif

#endif // WEATHER_SERVICE_H
