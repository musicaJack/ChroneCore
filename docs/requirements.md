# ChroneCore 需求说明

## 1. 产品概述

**ChroneCore** 是一款运行在 **M5Stack Core2** 上的多功能智能电子钟固件。以工程 **2048_M5Stack_Core2** 为软件基准（ESP-IDF 5.5+、官方 BSP、LVGL 8），集成 **Core2-for-AWS-IoT-Kit** 所覆盖的板载外设能力，并复用 **HourChime** 中成熟的 **WiFi 热点配网 + 配网页城市选择 + 心知天气 API** 方案。

### 1.1 目标用户与场景

- 桌面/床头时钟：数字或模拟表盘，显示日期与星期。
- 出行前查看当地天气（联网后自动刷新）。
- 厨房/运动等场景使用秒表。
- 多组闹钟提醒（声音 + 可选 LED/震动）。

### 1.2 硬件平台

| 项 | 说明 |
|---|---|
| 设备 | M5Stack Core2（ESP32，320×240 ILI9341，FT6336U 触摸） |
| 软件基准 | `2048_M5Stack_Core2` 构建与 BSP 初始化方式 |
| 扩展能力来源 | `Core2-for-AWS-IoT-Kit` 的 `core2forAWS` 驱动逻辑（需移植/适配至 IDF 5.x） |

> **说明**：AWS IoT Kit 官方固件基于 ESP-IDF 4.2 + LVGL 7；ChroneCore 以 2048 工程为主干，硬件驱动按芯片数据手册与 AWS BSP 行为在 `components/chrone_hal/` 中重新封装，不直接混用 LVGL 7 代码。

---

## 2. 功能需求

### 2.1 时钟显示（FR-CLOCK）

**视觉参考（必读 [clock-ui-reference.md](clock-ui-reference.md)）：**

| 模式 | 参考工程 | 要点 |
|------|----------|------|
| 数字 | **NanoTimer** | 顶栏日期/星期 + 七段数码管 `HH:MM:SS`；秒变化刷新 |
| 模拟 | **DS3231_Clock** | 黑底表盘、60 刻度、1–12 数字、银白时针/分针、红秒针；底栏日期/星期；指针擦除重绘 |

| ID | 需求 | 优先级 | 验收要点 |
|----|------|--------|----------|
| FR-CLOCK-01 | 支持 **数字时钟** 模式：时:分:秒，24 小时制（可配置 12 小时制为后续增强） | P0 | 样式对齐 NanoTimer 七段布局（横屏 320×240 适配）；秒级刷新 |
| FR-CLOCK-02 | 支持 **模拟时钟** 模式：表盘 + 时/分/秒针 | P0 | 样式对齐 DS3231_Clock 配色与指针算法；切换后 1s 内重绘 |
| FR-CLOCK-03 | 显示 **日期**（yyyy-MM-dd 或本地化格式） | P0 | 数字模式顶栏；模拟模式底栏（可参考 DS3231 `MM/DD`） |
| FR-CLOCK-04 | 显示 **星期**（中文：星期一…星期日） | P0 | 由 `struct tm` 计算（非 DS3231 星期寄存器） |
| FR-CLOCK-05 | 数字/模拟模式可切换（设置页或手势） | P1 | 选择持久化到 NVS |
| FR-CLOCK-06 | 无网络时以 **BM8563 RTC** 维持走时；有网络后 **SNTP** 校准 | P0 | 断电重启后时间偏差可接受（见 NFR） |
| FR-CLOCK-07 | 模拟分针默认 **按整分跳动**（与 DS3231 `kMinuteHandSweep=false` 一致）；设置中可选平滑分针 | P2 | 配置项可选 |

### 2.2 闹钟（FR-ALARM）

| ID | 需求 | 优先级 | 验收要点 |
|----|------|--------|----------|
| FR-ALARM-01 | 至少支持 **3 组** 独立闹钟（可扩展至 8 组） | P0 | 每组：开关、时、分、重复（一次/每天/工作日） |
| FR-ALARM-02 | 闹钟触发时：**扬声器** 播放提示音（可复用提示 OGG/WAV 或蜂鸣） | P0 | 触发后 30s 内可 **任意触摸** 或 **摇一摇** 停止 |
| FR-ALARM-03 | 可选：触发时 **SK6812** 灯条呼吸/闪烁提醒 | P1 | 不阻塞 UI 任务 |
| FR-ALARM-04 | 可选：触发时 **震动马达** 短振（AXP192 LDO3） | P2 | 与 2048 `vibration` 组件一致 |
| FR-ALARM-05 | 闹钟配置写入 **NVS**，掉电保持 | P0 | 升级固件后配置保留 |
| FR-ALARM-06 | 贪睡（Snooze）5/10 分钟 | P2 | 后续版本 |

### 2.3 天气预报（FR-WEATHER）

| ID | 需求 | 优先级 | 验收要点 |
|----|------|--------|----------|
| FR-WEATHER-01 | 通过 **心知天气（Seniverse）** API 获取当前天气 | P0 | HTTPS，`/v3/weather/now.json` |
| FR-WEATHER-02 | 在 **WiFi 配网热点 Web 页** 的「高级选项」中选择城市 | P0 | 行为对齐 HourChime |
| FR-WEATHER-03 | 城市列表为固件 **白名单**（拼音 location id + 中文名），写入 NVS 键 `wx_city`（namespace `weather`） | P0 | 非法城市拒绝保存 |
| FR-WEATHER-04 | 主界面展示：城市名、天气现象、温度（℃） | P0 | WiFi 未连接时显示占位 |
| FR-WEATHER-05 | 联网后按周期自动刷新（默认 30～60 分钟，可配置） | P1 | 失败退避重试 |
| FR-WEATHER-06 | API Key 通过 Kconfig 或 `sdkconfig.defaults` 注入，**不硬编码提交密钥** | P0 | 仓库内仅占位符 |

### 2.4 WiFi 与配网（FR-WIFI）

| ID | 需求 | 优先级 | 验收要点 |
|----|------|--------|----------|
| FR-WIFI-01 | **STA 模式** 连接路由器 | P0 | 保存 SSID/密码 |
| FR-WIFI-02 | 无已存 WiFi 或连接失败时进入 **AP 热点配网**（Web 配置） | P0 | 移植 HourChime `esp-wifi-connect` 流程 |
| FR-WIFI-03 | 配网热点 SSID 前缀可配置（如 `ChroneCore-` + MAC 后缀） | P1 | 与 HourChime `Hourchime-` 类似 |
| FR-WIFI-04 | 开机长按/组合键进入 **强制配网**（`force_ap` 标志 + 重启） | P1 | 参考 HourChime `ResetWifiConfiguration` |
| FR-WIFI-05 | 配网页 **高级选项** 含天气城市下拉（与 HourChime 一致） | P0 | GET `/advanced/config`，POST `/advanced/submit` |

### 2.5 秒表（FR-STOPWATCH）

| ID | 需求 | 优先级 | 验收要点 |
|----|------|--------|----------|
| FR-STOPWATCH-01 | 独立 **秒表界面**，精度 **1/10 秒或 1 秒**（P0 采用 1/10 秒显示，内部 1ms 计时） | P0 | 显示格式参考 NanoTimer：`MM:SS.cs` / `HH:MM:SS`；最大 99:59:59 |
| FR-STOPWATCH-02 | **自定义按键**：启动、暂停、重置 | P0 | 见 §2.6；逻辑参考 NanoTimer `Stopwatch::toggle/reset` |
| FR-STOPWATCH-03 | 状态：`IDLE` → `RUNNING` ↔ `PAUSED` → `IDLE`（重置） | P0 | `base_ms + (now - started_at)`，与 NanoTimer 一致 |
| FR-STOPWATCH-04 | 从主界面进入/退出秒表，不丢失时钟走时 | P0 | 后台时钟任务独立 |
| FR-STOPWATCH-05 | 运行中 UI 刷新周期 **≤50ms**（NanoTimer 为 50ms） | P1 | 百分秒可见变化 |

### 2.6 按键与交互（FR-INPUT）

Core2 无独立机械按键，采用 **触摸屏自定义区域 + 可选底部虚拟三键**（对齐 AWS `button_left/middle/right` 区域）。

| 按键/区域 | 秒表模式 | 主界面/其他 |
|-----------|----------|-------------|
| **左区**（屏宽 0～30%） | 重置 | 上一页/减小 |
| **中区**（30%～70%） | 启动 / 暂停（toggle） | 确认 / 进入菜单 |
| **右区**（70%～100%） | （保留） | 下一页/增加 |
| **上滑/下滑** | — | 切换功能页（时钟/天气/闹钟/设置） |
| **左+右同时按下**（底部虚拟键） | — | **进入闹钟配置**（阶段 4；响铃中不进入） |

| ID | 需求 | 优先级 |
|----|------|--------|
| FR-INPUT-01 | 秒表三操作必须由上述三区或屏内 **LVGL 按钮** 完成（至少一种，建议两者并存） | P0 |
| FR-INPUT-02 | 触摸防抖 50～100ms，避免误触 | P1 |
| FR-INPUT-03 | UI 绘制遵循 `bsp_display_lock` / LVGL 线程安全（2048 惯例） | P0 |

### 2.7 外设集成（FR-HW）

以下能力须在 ChroneCore 中 **可用且文档化**（实现阶段可分优先级落地）：

| 外设 | 来源参考 | 功能用途（本产品） | 优先级 |
|------|----------|-------------------|--------|
| SK6812×10/侧 | AWS `Core2ForAWS_Sk6812_*` / 2048 `sk6812_led` | 闹钟/配网状态指示 | P1 |
| MPU6886 | AWS `MPU6886_*` / 2048 `imu_mpu6886` | 摇一摇关闹钟、亮屏唤醒（可选） | P2 |
| BM8563 | AWS `BM8563_*` | 主时钟源、闹钟调度 | P0 |
| SPM1423 麦克风 | AWS `Microphone_*` | 声学配网（可选）、未来语音 | P3 |
| NS4168 扬声器 | AWS `Core2ForAWS_Speaker_*` | 闹钟铃声 | P0 |
| TF 卡 | AWS `Core2ForAWS_SDcard_*` | 自定义铃声、日志（可选） | P3 |

**硬件约束（必须遵守）：**

- 麦克风与扬声器 **不能同时使能**（共用 GPIO0，AWS 文档明确会 hard fault）。
- TF 卡与显示屏 **共用 SPI**，访问 SD 前必须获取 **`spi_mutex`** 并 `spi_poll()`（AWS BSP 约定）。

---

## 3. 非功能需求（NFR）

| ID | 类别 | 要求 |
|----|------|------|
| NFR-01 | 性能 | UI 刷新：时钟界面 ≥1Hz；全屏切换 <300ms |
| NFR-02 | 内存 | 保持 2048 级堆余量 >150KB；大缓冲放 SPIRAM（若启用） |
| NFR-03 | 时间精度 | SNTP 同步后误差 <2s；仅 RTC 时漂移 <20ppm（BM8563 级） |
| NFR-04 | 可靠性 | WiFi/HTTP 失败不导致主时钟停走 |
| NFR-05 | 安全 | 心知 API Key 仅本地编译配置；HTTPS 使用证书校验 |
| NFR-06 | 可维护 | 模块边界清晰：`hal` / `services` / `ui` / `app` |
| NFR-07 | 升级 | 支持 ESP-IDF OTA（阶段三后） |

---

## 4. 界面与信息架构（概要）

```
主界面（时钟）
  ├─ 数字表盘 / 模拟表盘
  ├─ 日期 + 星期
  └─ 天气条（城市、图标、温度）

侧滑/菜单
  ├─ 闹钟列表
  ├─ 秒表
  ├─ 设置（时钟样式、亮度、WiFi、关于）
  └─ 配网入口（force AP）
```

---

## 5. 范围外（Out of Scope）— 初版

- Alexa / AWS IoT Shadow / RainMaker 云端控制
- MCP、大模型语音对话（HourChime 主工程能力）
- 多时区世界时钟（可作 v2）
- 农历、节气（可作 v2）
- 2048 游戏本体

---

## 6. 依赖与假设

1. 用户拥有可连接的 2.4GHz WiFi 与有效心知 API Key。
2. 固件以 **标准 M5Stack Core2** 为目标；若使用 Core2 for AWS 版本（含 ATECC608），额外安全芯片 **初版不使用**。
3. HourChime 的 `78/esp-wifi-connect` 组件将 **移植或作为 idf 组件依赖** 引入 ChroneCore，并适配 LVGL/显示提示方式（2048 无 `xGuiSemaphore`，改用 `bsp_display_lock`）。
4. 数字/模拟表盘 **视觉与刷新算法** 分别从 **NanoTimer**、**DS3231_Clock** 移植，详见 [clock-ui-reference.md](clock-ui-reference.md)；两工程为 RP2040 + 不同分辨率，须在 Core2 320×240 上重新定标，**不**照搬 GPIO/DS3231 驱动。

---

## 7. 验收总览（V1.0）

- [ ] 数字/模拟时钟正确显示日期、星期、走时
- [ ] BM8563 + SNTP 协同，断网/联网场景时间合理
- [ ] 至少 3 组闹钟可设置并触发发声
- [ ] 热点配网成功，高级页选择城市后心知天气显示正确
- [ ] 秒表启动/暂停/重置按键逻辑正确
- [ ] SK6812 可选提醒；麦克风/扬声器不同时开启
- [ ] 文档与代码模块划分符合 architecture.md
