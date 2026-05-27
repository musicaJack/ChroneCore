# ChroneCore API 参考

本文档汇总 ChroneCore 开发所依赖的 **参考工程 API**、**拟自建模块接口** 与 **外部 HTTP API**。实现时以仓库 `components/` 与 `main/` 中头文件为准；本文件为设计期契约。

---

## 1. 参考工程 API 对照

### 1.1 基准工程：2048_M5Stack_Core2

| 模块 | 头文件/路径 | ChroneCore 用途 |
|------|-------------|-----------------|
| 显示 + LVGL | `bsp/esp-bsp.h`, `bsp/touch.h` | 主 UI、触摸 |
| 电源 | `components/axp192_esp32/axp192.h` | 背光、震动 LDO、供电 |
| 震动 | `components/vibration/vibration.h` | 闹钟振动 |
| 触摸方向 | `main/touch_direction*.` | 可复用为滑动手势 |
| NVS | `nvs_flash.h` | 配置持久化 |
| 游戏循环模式 | `render_requested` + 独立任务 | 借鉴为 UI 刷新与后台服务分离 |

**显示初始化（沿用 2048 模式）：**

```c
bsp_display_cfg_t cfg = {
    .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
    .buffer_size = 320 * 40,
    .double_buffer = true,
    .flags = { .buff_dma = true, .buff_spiram = false },
};
lv_display_t *disp = bsp_display_start_with_config(&cfg);
bsp_display_brightness_set(60);

esp_lcd_touch_handle_t touch;
bsp_touch_new(&(bsp_touch_config_t){0}, &touch);
```

**LVGL 互斥：**

```c
bsp_display_lock(timeout_ms);
/* lv_xxx */
bsp_display_unlock();
```

### 1.2 硬件能力：Core2-for-AWS-IoT-Kit (`core2forAWS`)

> 原始 API 面向 ESP-IDF 4.2 + LVGL 7 + `xGuiSemaphore`。ChroneCore 在 `chrone_hal` 中提供 **语义等价** 的 IDF 5.x 封装，下表为 **逻辑 API**（移植目标）。

#### 1.2.1 初始化

```c
void Core2ForAWS_Init(void);  /* 目标：chrone_hal_init() */
```

依次使能（Kconfig 控制）：AXP192、ILI9341、FT6336U、SK6812、MPU6886、BM8563、ATECC608（可选，本产品默认关闭）。

#### 1.2.2 显示与背光

```c
void Core2ForAWS_Display_Init(void);
void Core2ForAWS_Display_SetBrightness(uint8_t brightness);  /* 0-100 */
```

ChroneCore **优先** 使用 2048 的 `bsp_display_brightness_set()`；仅在未走 BSP 时回退 PMU 背光。

#### 1.2.3 SK6812 RGB 灯条（10 颗，底部两侧）

```c
void Core2ForAWS_Sk6812_Init(void);
void Core2ForAWS_Sk6812_SetColor(uint16_t pos, uint32_t color);       /* pos: 0-9 */
void Core2ForAWS_Sk6812_SetSideColor(uint8_t side, uint32_t color); /* SK6812_SIDE_LEFT/RIGHT */
void Core2ForAWS_Sk6812_SetBrightness(uint8_t brightness);
void Core2ForAWS_Sk6812_Show(void);
void Core2ForAWS_Sk6812_Clear(void);
```

**2048 替代组件**（已存在，GPIO25，RMT）：

```c
/* components/sk6812_led/sk6812_led.h */
esp_err_t sk6812_led_init(void);
esp_err_t sk6812_led_set_color(uint16_t index, sk6812_rgb_t color);
esp_err_t sk6812_led_set_side_color(uint8_t side, sk6812_rgb_t color);
esp_err_t sk6812_led_refresh(void);
```

**ChroneCore 建议：** 统一为 `chrone_led_*` 薄封装，默认调用 `sk6812_led`。

#### 1.2.4 BM8563 RTC

```c
/* AWS: bm8563.h */
typedef struct {
    uint16_t year;
    uint8_t month, day, hour, minute, second;
} rtc_date_t;

void BM8563_Init(void);
void BM8563_GetTime(rtc_date_t *date);
void BM8563_SetTime(const rtc_date_t *date);
```

**ChroneCore 服务层：**

```c
/* components/bm8563 + chrone_time — 已实现 */
esp_err_t bm8563_init(void);
esp_err_t bm8563_get_time(bm8563_datetime_t *out);
esp_err_t bm8563_set_time(const bm8563_datetime_t *in);

esp_err_t chrone_time_init(void);
esp_err_t chrone_time_get_local_tm(struct tm *out);
esp_err_t chrone_time_set_local_tm(const struct tm *in);
bool chrone_time_rtc_synced(void);
const char *chrone_time_weekday_zh(int tm_wday);
esp_err_t chrone_time_sync_sntp_start(void);  /* 阶段 3 WiFi 后调用 */
bool chrone_time_sntp_synced(void);
```

#### 1.2.5 MPU6886 IMU

```c
void MPU6886_Init(void);
void MPU6886_GetAccelData(float *ax, float *ay, float *az);
void MPU6886_GetGyroData(float *gx, float *gy, float *gz);
```

**2048：** `components/imu_mpu6886/`（2048 主程序未启用，可直接移植）。

**ChroneCore（可选）：**

```c
esp_err_t chrone_imu_init(void);
esp_err_t chrone_imu_read_accel(float *ax, float *ay, float *az);
bool chrone_imu_shake_detected(void);  /* 关闹钟 */
```

#### 1.2.6 麦克风 SPM1423（I2S）

```c
void Microphone_Init(void);
/* 配合 i2s_read() 采集 PCM */
```

**约束：** 使用前 `Core2ForAWS_Speaker_Enable(0)`，且勿与扬声器并发。

**HourChime/ AWS Smart-Thermostat 参考：** `microphone_task` + FFT 仅用于噪声；ChroneCore 初版 **不启用**，除非做声学配网。

#### 1.2.7 扬声器 NS4168（I2S）

```c
void Core2ForAWS_Speaker_Enable(uint8_t state);  /* 1=开, 0=关 */
```

**ChroneCore：**

```c
esp_err_t chrone_audio_init(void);
esp_err_t chrone_audio_play_alarm(const uint8_t *pcm, size_t len);
esp_err_t chrone_audio_play_tone(uint16_t freq_hz, uint16_t duration_ms);
void chrone_audio_deinit(void);
```

#### 1.2.8 TF 卡（SPI 与屏共用）

```c
extern SemaphoreHandle_t spi_mutex;  /* AWS BSP */

esp_err_t Core2ForAWS_SDcard_Mount(const char *mount_path, sdmmc_card_t **out_card);
esp_err_t Core2ForAWS_SDcard_Unmount(const char *mount_path, sdmmc_card_t *card);
```

**使用模板：**

```c
xSemaphoreTake(spi_mutex, portMAX_DELAY);
spi_poll();
/* fopen / fread 于 /sdcard/... */
xSemaphoreGive(spi_mutex);
```

ChroneCore 若采用 2048 单一 BSP SPI 管理，需在 `chrone_hal_sdcard` 中与 `bsp_display_lock` **统一总线仲裁**（避免与 AWS 双 mutex 冲突）。

#### 1.2.9 虚拟按键（触摸三区）

```c
extern Button_t *button_left, *button_middle, *button_right;
bool Button_WasPressed(Button_t *btn);
```

ChroneCore 秒表映射见 [requirements.md](requirements.md) §2.6。

---

### 1.3 联网与天气：HourChime

#### 1.3.1 WiFi 组件（`78/esp-wifi-connect`）

| 类/API | 职责 |
|--------|------|
| `WifiStation` | STA 扫描、连接、`WaitForConnected` |
| `WifiConfigurationAp` | 开 AP、`Start()`、内置 Web 服务器 |
| `SsidManager` | 已保存 SSID 列表 |

**配网流程（逻辑）：**

```
若无 SSID 或连接超时 → WifiConfigurationAp::Start()
用户浏览器打开 http://192.168.4.1/ → 提交 SSID/密码
可选高级页 → weather_location → POST /advanced/submit
完成后设备重启或 STA 重连
```

**强制配网：**

```c
Settings("wifi").SetInt("force_ap", 1);
esp_restart();
```

ChroneCore 移植为 C API 或引入 HourChime 组件（推荐 **idf_component.yml** 依赖 `78/esp-wifi-connect`，C++ 包装层 `chrone_wifi.cpp`）。

#### 1.3.2 配网页 HTTP API（高级选项）

| 方法 | URI | 说明 |
|------|-----|------|
| GET | `/advanced/config` | 返回 `weather_location`（当前）、`weather_cities[]`（`{id,name_zh}` 列表） |
| POST | `/advanced/submit` | JSON 含 `weather_location`，服务端白名单校验后写 NVS |

**实现位置（HourChime）：** `components/78__esp-wifi-connect/wifi_configuration_ap.cc`（`CONFIG_APP_VOICE_CLOCK` 宏保护）。

**ChroneCore：** 定义 `CONFIG_CHRONECORE_WEATHER_CONFIG=y`，复用同一套 URI 与 HTML 片段（`assets/wifi_configuration.html`）。

#### 1.3.3 城市列表（固件白名单）

```c
/* app/weather/weather_city_list.h — 移植到 components/chrone_weather/ */
typedef struct {
    const char *id;       /* 心知 location，如 "beijing" */
    const char *name_zh;  /* 配网页显示 */
} weather_city_entry_t;

size_t weather_city_get_count(void);
const weather_city_entry_t *weather_city_get_entry(size_t index);
bool weather_city_is_valid_id(const char *id);
const char *weather_city_get_name_zh(const char *id);
void weather_city_format_title_case(const char *id, char *out, size_t out_sz);
```

#### 1.3.4 NVS 键（天气）

| Namespace | Key | 值 | 说明 |
|-----------|-----|-----|------|
| `weather` | `wx_city` | 如 `shanghai` | 心知 location id（**键名≤15 字符**） |
| `wifi` | `force_ap` | 0/1 | 强制进入 AP 配网 |

配网页 JSON 字段名仍为 `weather_location`，与 NVS 键 `wx_city` 映射在提交处理中完成。

#### 1.3.5 天气服务 API（拟建 `chrone_weather`）

```c
typedef struct {
    char city[32];
    char text[32];
    char temp[8];
    int code;
    bool available;
} chrone_weather_info_t;

chrone_weather_info_t *chrone_weather_get_info(void);
void chrone_weather_start_query(void);
void chrone_weather_get_display_city(char *buf, size_t buf_sz);
```

实现参考：`HourChime/app/weather/weather_service.c`（`esp_http_client` + `cJSON` + `esp_crt_bundle`）。

---

## 2. 心知天气（Seniverse）HTTP API

### 2.1 实时天气

```
GET https://api.seniverse.com/v3/weather/now.json
```

| 参数 | 必填 | 说明 |
|------|------|------|
| `key` | 是 | API Key（Kconfig `CONFIG_CHRONE_WEATHER_API_KEY`） |
| `location` | 是 | NVS `wx_city` 或默认 `CONFIG_CHRONE_WEATHER_DEFAULT_CITY` |
| `language` | 否 | `zh-Hans` / `en` |
| `unit` | 否 | `c` 摄氏度 |

**响应（节选）：**

```json
{
  "results": [{
    "location": { "name": "上海", "id": "..." },
    "now": { "text": "多云", "code": "4", "temperature": "26" }
  }]
}
```

### 2.2 location 规则

- 使用 **小写拼音** 或 `省拼音 城市拼音`（如 `jiangsu taizhou`）。
- 与 `weather_city_list.c` 中 `id` **精确匹配**。
- 详见 HourChime `docs/WEATHER_CITY_CONFIG_SOLUTION.md`。

### 2.3 配置头（模板）

```c
/* chrone_weather_config.h */
#define CHRONE_WEATHER_HOST  "https://api.seniverse.com"
/* KEY/CITY 由 Kconfig 生成，勿提交真实 key */
```

---

## 3. ChroneCore 自建模块 API（规划）

### 3.1 应用状态机 `chrone_app`

```c
typedef enum {
    CHRONE_SCREEN_CLOCK = 0,
    CHRONE_SCREEN_ALARM,
    CHRONE_SCREEN_STOPWATCH,
    CHRONE_SCREEN_SETTINGS,
    CHRONE_SCREEN_WIFI_PROV,
} chrone_screen_t;

void chrone_app_init(void);
void chrone_app_set_screen(chrone_screen_t screen);
chrone_screen_t chrone_app_get_screen(void);
```

### 3.2 闹钟 `chrone_alarm`

```c
#define CHRONE_ALARM_MAX 4

typedef enum {
    CHRONE_ALARM_REPEAT_DAILY = 0,
    CHRONE_ALARM_REPEAT_WORKDAY,
    CHRONE_ALARM_REPEAT_ONCE,
} chrone_alarm_repeat_t;

typedef struct {
    bool enabled;
    uint8_t hour, minute;
    uint8_t repeat;     /* chrone_alarm_repeat_t */
    uint8_t flags;      /* reserved */
} chrone_alarm_t;

esp_err_t chrone_alarm_load(void);
esp_err_t chrone_alarm_save(void);
esp_err_t chrone_alarm_set(uint8_t index, const chrone_alarm_t *alarm);
bool chrone_alarm_scheduling_enabled(void);
uint8_t chrone_alarm_enabled_count(void);  /* 表盘图标角标 */
bool chrone_alarm_once_time_ok(uint8_t hour, uint8_t minute);  /* Once 保存：≥2 分钟后 */
void chrone_alarm_check_tick(const struct tm *now);  /* 每秒调用 */
```

### 3.3 秒表 `chrone_stopwatch`

逻辑对齐 **NanoTimer** `services/stopwatch.cpp`（`base_ms_` + `started_at_ms_`）。

```c
typedef enum {
    CHRONE_SW_IDLE = 0,
    CHRONE_SW_RUNNING,
    CHRONE_SW_PAUSED,
} chrone_stopwatch_state_t;

void chrone_stopwatch_reset(void);
void chrone_stopwatch_start_pause(void);  /* toggle，同 Stopwatch::toggle */
chrone_stopwatch_state_t chrone_stopwatch_get_state(void);
void chrone_stopwatch_get_elapsed_ms(int64_t *out_ms);

/* UI 格式化 — 参考 ui_stopwatch.cpp format_elapsed */
void chrone_stopwatch_format_display(int64_t elapsed_ms, char *buf, size_t len);
```

**按键绑定：**

```c
void chrone_stopwatch_on_button(chrone_button_id_t id);
/* CHRONE_BTN_LEFT  → reset */
/* CHRONE_BTN_MID   → start/pause */
```

### 3.4 UI `chrone_ui`

详见 [clock-ui-reference.md](clock-ui-reference.md)。

#### 数字时钟（移植自 NanoTimer `ui_clock.cpp`）

```c
/* 七段绘制 — segment_draw.c */
void segment_draw_digit(lv_canvas_t *c, int x, int y, int digit);  /* 0-9 */
void segment_draw_colon(lv_canvas_t *c, int x, int y);

/* 数字表盘 — clock_digital.c */
void chrone_ui_clock_digital_paint(const struct tm *tm, const chrone_weather_info_t *w);
```

**参考常量：** `kSegmentPatterns[10]`、`kHeaderHeight`、digit pitch/gap（横屏需重算，见 clock-ui-reference §1.4）。

#### 模拟时钟（移植自 DS3231_Clock `AnalogClockView`）

```c
/* 图元 — gfx_primitives.c，来自 gfx_clock.cpp */
void chrone_gfx_fill_rect(int x, int y, int w, int h, uint16_t color565);
void chrone_gfx_draw_line(int x0, int y0, int x1, int y1, int thick, uint16_t color565);
void chrone_gfx_draw_circle(int cx, int cy, int radius, int thick, uint16_t color565);

void chrone_ui_clock_analog_init(lv_obj_t *parent);
void chrone_ui_clock_analog_paint_static_dial(void);
void chrone_ui_clock_analog_on_second_tick(const struct tm *tm);
void chrone_ui_clock_analog_refresh_calendar(const struct tm *tm);

/* 指针角度（度，12 点顺时针）— 与 analog_clock.cpp 一致 */
float chrone_clock_angle_second(int sec);
float chrone_clock_angle_minute(int min, int sec, bool sweep);
float chrone_clock_angle_hour(int hour, int min, int sec);
```

**参考配色：** `dial_palette`（`kFace`, `kHandSecond=RED` 等），见 clock-ui-reference §2.3。

#### 统一入口

```c
void chrone_ui_init(lv_obj_t *root);
void chrone_ui_set_mode_digital(bool digital);
void chrone_ui_refresh_clock(const struct tm *tm, const chrone_weather_info_t *w);
void chrone_ui_refresh_stopwatch(int64_t elapsed_ms, chrone_stopwatch_state_t st);
```

### 3.5 输入 `chrone_input`

```c
typedef enum {
    CHRONE_BTN_LEFT,
    CHRONE_BTN_MIDDLE,
    CHRONE_BTN_RIGHT,
} chrone_button_id_t;

typedef void (*chrone_button_cb_t)(chrone_button_id_t btn, void *user_data);

esp_err_t chrone_input_init(esp_lcd_touch_handle_t touch);
void chrone_input_set_screen_handler(chrone_screen_t screen, chrone_button_cb_t cb);
```

---

## 4. Kconfig 规划（摘要）

| 符号 | 说明 |
|------|------|
| `CONFIG_CHRONE_WEATHER_API_KEY` | 心知 Key |
| `CONFIG_CHRONE_WEATHER_DEFAULT_CITY` | 默认 `shanghai` |
| `CONFIG_CHRONE_WIFI_AP_PREFIX` | 默认 `ChroneCore` |
| `CONFIG_CHRONE_HAL_SK6812` | 启用灯条 |
| `CONFIG_CHRONE_HAL_MPU6886` | 启用 IMU |
| `CONFIG_CHRONE_HAL_SDCARD` | 启用 TF |
| `CONFIG_CHRONE_WEATHER_CONFIG` | 配网页城市选项 |

---

## 5. 冲突与适配备忘

| 冲突点 | 处理 |
|--------|------|
| LVGL 7 vs 8 | 仅使用 2048 / BSP 的 LVGL 8 API |
| `xGuiSemaphore` vs `bsp_display_lock` | 全部 UI 用 BSP 锁 |
| 麦/喇叭 GPIO0 | 闹钟播放前 `Speaker_Enable(1)`，结束后关闭；麦克风默认关闭 |
| SPI 屏 vs SD | 单一 `chrone_spi_bus_lock()` 封装 |
| HourChime C++ WiFi | ChroneCore 可用 C++ 文件链接组件，或 C 包装 |

---

## 6. 参考文件索引

| 功能 | 路径 |
|------|------|
| 2048 主入口 | `2048_M5Stack_Core2/main/main.c` |
| AWS BSP 头文件 | `Core2-for-AWS-IoT-Kit/Factory-Firmware/components/core2forAWS/core2forAWS.h` |
| AWS RTC UI | `Core2-for-AWS-IoT-Kit/Factory-Firmware/main/clock.c` |
| **数字时钟 UI** | `NanoTimer/src/ui/ui_clock.cpp` |
| **七段字模** | `NanoTimer/src/ui/ui_clock.cpp` → `kSegmentPatterns` |
| **秒表 UI/逻辑** | `NanoTimer/src/ui/ui_stopwatch.cpp`, `src/services/stopwatch.cpp` |
| **模拟表盘** | `DS3231_Clock/src/clock/analog_clock.cpp` |
| **表盘图元** | `DS3231_Clock/src/clock/gfx_clock.cpp` |
| HourChime 天气 | `HourChime/app/weather/weather_service.c` |
| HourChime 城市表 | `HourChime/app/weather/weather_city_list.c` |
| HourChime WiFi 文档 | `HourChime/app/docs/WIFI_CONFIG_AND_PORTING.md` |
| 配网 Web | `HourChime/components/78__esp-wifi-connect/wifi_configuration_ap.cc` |
| 时钟 UI 移植说明 | [clock-ui-reference.md](clock-ui-reference.md) |
