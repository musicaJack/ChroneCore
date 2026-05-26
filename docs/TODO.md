# ChroneCore 开发待办

**架构：** C HAL + C++17 应用/UI/联网 · **基准：** 2048_M5Stack_Core2

状态图例：`[ ]` 待办 · `[~]` 进行中 · `[x]` 完成

---

## 阶段 0 — 工程脚手架

- [x] 文档：需求 / API / 架构 / 开发计划 / 时钟 UI 参考
- [x] 文档：混合架构语言选型写入 architecture.md
- [x] 从 2048 复制 `axp192_esp32`、`vibration`、`sdkconfig.defaults`
- [x] 根 `CMakeLists.txt`、`partitions.csv`、`main/idf_component.yml`
- [x] `chrone_hal`（C）+ `chrone_app`（C++17）+ `main.cpp` 最小启动屏
- [x] `idf.py set-target esp32` + `idf.py build` 通过
- [x] 实机：串口日志 + 屏幕显示

---

## 阶段 1 — 时间与 RTC

- [x] 组件 `bm8563`（I2C 0x51，BSP `I2C_NUM_1` / GPIO21-22）
- [x] 组件 `chrone_time`：RTC → `settimeofday`，中文星期
- [x] 主屏 1Hz 显示 `HH:MM:SS` + 日期星期（`clock_screen.cpp`）
- [x] SNTP 接口预留（`chrone_time_sync_sntp_start`，阶段 3 联网后调用）
- [x] 实机：BM8563 时间与串口日志一致；烧录验证

---

## 阶段 2 — 时钟 UI

- [x] `chrone_ui` 组件（C++17）
- [x] 数字钟：DSEG7 TTF + LVGL 标签局部刷新
- [x] 触摸点击切换数字/模拟（NVS `clk_mode` 持久化）
- [x] 模拟钟：DS3231_Clock 风格 Canvas 表盘 + 时分秒针局部刷新
- [x] 320×240 布局标定（`clock_layout.h`）
- [x] NVS `clk_mode` 读写
- [x] 1 Hz LVGL 秒 tick + 局部刷新

---

## 阶段 3 — WiFi 与天气

- [x] `chrone_esp_wifi_connect`（自 HourChime `78/esp-wifi-connect` 移植 + 城市高级页）
- [x] `chrone_wifi`：STA / AP 配网 / `force_ap`
- [x] `chrone_weather`：城市白名单 + 心知 HTTPS
- [x] Kconfig：`CHRONECORE_WEATHER_API_KEY`、默认城市
- [x] 顶栏天气 + AP 配网全屏提示 UI
- [x] 实机：配网 + SNTP 校时 + 天气（城市/温度）显示
- [x] 布局方案 A：顶栏日期+英文星期，底栏图标+英文现象+温度+城市（见 [clock-weather-layout.md](clock-weather-layout.md)）
- [x] 移植 HourChime 天气图标：`weather_icon_map` + `icons_48x48/` + 1-bit→RGB565（24×24 显示）
- [x] `chrone_ui` 底栏天气带 + `weather_info.text` / `code`；界面仅英文，不用中文
- [x] 顶栏星期统一 `chrone_time_weekday_en`（去掉中文星期显示）
- [ ] 实机：底栏图标与 Cloudy/Rainy 文案显示验收

---

## 阶段 4 — 闹钟与音频

- [ ] `chrone_alarm` NVS 多组闹钟
- [ ] 扬声器播放（AWS/ESP-IDF I2S，与麦克风互斥）
- [ ] 闹钟 UI + 到点响铃/停止
- [ ] 可选：震动 + SK6812 提醒

---

## 阶段 5 — 秒表与按键

- [ ] `chrone_stopwatch`（C++，NanoTimer `Stopwatch`）
- [ ] `stopwatch_ui` + 50ms 刷新
- [ ] `chrone_input` 触摸三区 / 屏内按钮
- [ ] 主菜单进入/退出秒表

---

## 阶段 6 — 收尾与 V1.0

- [ ] SK6812 状态指示（可选）
- [ ] MPU6886 摇一摇关闹钟（可选）
- [ ] TF 卡 + SPI 互斥（可选）
- [ ] 设置页：亮度、强制配网、关于
- [ ] 全量测试表（见 development-plan.md §10）
- [ ] 根 README 快速开始（烧录说明）

---

## 后续（V1.1+）

- [ ] OTA
- [ ] 多时区 / 农历
- [ ] 声学配网
- [ ] TF 卡自定义铃声
