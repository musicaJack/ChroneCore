# ChroneCore

面向 **M5Stack Core2** 的多功能智能闹钟：数字/模拟时钟 UI、天气预报、秒表与多组闹钟，支持 WiFi 热点配网与心知天气。

## 设计文档

完整需求、API、架构与开发计划见 **[docs/](docs/)**：

| 文档 | 说明 |
|------|------|
| [docs/requirements.md](docs/requirements.md) | 功能与非功能需求 |
| [docs/api-reference.md](docs/api-reference.md) | 参考工程 API 与自建模块接口 |
| [docs/architecture.md](docs/architecture.md) | 分层架构、状态机、NVS |
| [docs/clock-ui-reference.md](docs/clock-ui-reference.md) | 数字/模拟时钟 UI 移植规格（NanoTimer、DS3231_Clock） |
| [docs/development-plan.md](docs/development-plan.md) | 分阶段实施与验收 |

**技术路线：** 以 [2048_M5Stack_Core2](C:/Users/sh_mu/code/2048_M5Stack_Core2) 为工程基准，**混合架构（C HAL + C++17 应用）**；集成 [Core2-for-AWS-IoT-Kit](C:/Users/sh_mu/code/Core2-for-AWS-IoT-Kit) 外设 API，复用 [HourChime](C:/Users/sh_mu/code/HourChime) 配网与心知天气；表盘 UI 参考 [NanoTimer](C:/Users/sh_mu/code/NanoTimer)、[DS3231_Clock](C:/Users/sh_mu/code/DS3231_Clock)。

## 快速构建（阶段 0 脚手架）

需已安装 ESP-IDF 5.5+ 并执行 `export.ps1` / `get_idf`：

```bash
cd ChroneCore
idf.py set-target esp32
idf.py build
idf.py -p COMx flash monitor
```

上电后屏幕应显示 **ChroneCore** 占位界面。进度见 [docs/TODO.md](docs/TODO.md)。

---

## 参考工程索引

| 工程 | 用途 |
|------|------|
| 2048_M5Stack_Core2 | ESP-IDF 5.5、BSP、LVGL、工程骨架 |
| Core2-for-AWS-IoT-Kit | SK6812、IMU、RTC、音频、TF 卡 |
| HourChime | WiFi 热点配网、心知天气、城市 NVS |
| **NanoTimer** | **数字钟**七段 UI、秒表逻辑 |
| **DS3231_Clock** | **模拟钟**表盘、指针刷新 |

## 参考调研（2048 工程）

本仓库曾记录同平台 **2048** 游戏工程的调研结论，供 BSP/LVGL 对照；产品实现以 **docs/** 为准。

**参考项目路径：** `C:/Users/sh_mu/code/2048_M5Stack_Core2`  
**参考项目版本：** v1.1.0（README 标注为稳定版）  
**文档整理日期：** 2026-05-26

---

## 参考项目概览

| 项 | 说明 |
|---|---|
| 平台 | M5Stack Core2（ESP32，README 亦提及 ESP32-S3 构建说明） |
| 框架 | ESP-IDF 5.5+、FreeRTOS |
| UI | LVGL 8.x + `espressif__esp_lvgl_port` |
| 硬件抽象 | `espressif__m5stack_core_2` BSP |
| 主程序 | `main/main.c`（`main/CMakeLists.txt` 当前仅编译此入口） |
| 许可证 | MIT |

从 **RP2040 版 2048** 移植而来，在 Core2 上实现完整可玩的触摸版 2048，并针对 320×240 屏做了 LVGL 渲染与局部刷新优化。

---

## 目录结构

```
2048_M5Stack_Core2/
├── main/
│   ├── main.c                          # 主入口：硬件初始化、游戏/UI/输入、游戏循环任务
│   ├── touch_direction.c / .h          # 触摸方向检测（C）
│   ├── touch_direction_recognizer.cpp  # 触摸轨迹方向识别（C++）
│   ├── font_renderer.c / .h            # 8×8 点阵字渲染（倒计时等）
│   └── font_8x8.c / .h                 # 字模数据
├── components/
│   ├── game2048/                       # 游戏核心组件
│   │   ├── src/game/                   # game_2048.c, game_logic.c, game_state.c
│   │   ├── src/ui/                     # ui_renderer.c, ui_colors.c
│   │   ├── src/input/                  # input_handler.cpp
│   │   └── include/game2048/           # 对外头文件
│   ├── axp192_esp32/                   # AXP192 电源管理（旧版 I2C API）
│   ├── vibration/                      # 震动马达（AXP192 LDO3）
│   ├── ili9342c_esp32/                 # 遗留显示驱动（主流程未使用）
│   ├── imu_mpu6886/                    # IMU（主流程未启用）
│   ├── sk6812_led/                     # RGB LED（主流程已移除）
│   └── joystick_esp32/                 # 摇杆（仅测试程序）
├── managed_components/                 # ESP-IDF 托管组件（BSP、LVGL、LCD、触摸等）
├── build_esp32.bat                     # Windows 一键编译烧录
├── README.md / README.zh.md
└── imgs/demo.jpg
```

---

## 已实现功能

### 游戏逻辑（`components/game2048`）

- **4×4 棋盘**，标准 2048 规则：四向移动、同值合并、无效移动不生成新块。
- **开局**：`game_logic_add_initial_tiles()` 固定生成 **2 个「2」**。
- **新块**：有效移动后 `game_logic_add_random_tile()`；默认配置为 **100% 生成 2**（`GAME_CONFIG_TILE_2_PROBABILITY = 100`）。
- **胜负**：达到 `win_value`（默认 **2048**）→ `game_2048_has_won()`；无可走步 → `game_2048_is_game_over()`。
- **状态**：`GAME_STATE_PLAYING` / `GAME_STATE_WON` / `GAME_STATE_GAME_OVER`。
- **分数**：合并加分；`best_score` 在内存中随当前分更新（`game_state_add_score`），**重置单局不清零 best_score**。
- **合并回调**：`merge_callback_t`，`game_2048_reset` 后会 **保留** 回调（用于震动等外设反馈）。

### 显示与 UI

- **320×240 ILI9341**，BSP + LVGL，双缓冲 + DMA（`buffer_size = 320 * 40`）。
- **棋盘 UI**：彩色方块、`ui_colors` 配色；默认布局约 224×224 棋盘，水平居中、距顶 4px（见 `ui_config_get_default()`）。
- **局部刷新**：`ui_renderer_render_game()` 对比 `ui_render_cache_t`，仅更新变化格子；另有 `ui_renderer_render_game_full()` 全量重绘。
- **启动倒计时**：`show_startup_countdown()`，8×8 字模放大 5 倍显示 `3 → 2 → 1 → GO`。
- **弹窗**：`ui_renderer_show_win_dialog` / `ui_renderer_show_gameover_dialog`。

### 输入与交互

| 场景 | 行为 |
|------|------|
| 正常对局 | 滑动手势 → 四向移动 |
| 胜利弹窗显示中 | 任意方向滑动 → 关闭弹窗，**继续当前局** |
| 失败弹窗显示中 | 滑动 → 关闭弹窗并 `game_2048_reset()` 开新局 |
| `WON` 或 `GAME_OVER` | 长按屏幕 **2 秒** → `esp_restart()` 软复位（由 `game_loop_task` 检测，非 input_handler） |

触摸链路：`bsp_touch_new` → `input_handler` → `TouchDirectionRecognizer`（C++）。

### 硬件与反馈

- **AXP192**：`GPIO21/22` I2C，`axp192_create/init`，为显示/触摸/外设供电；主程序中亦配置 DC-DC1 供 MPU（遗留，IMU 未启用）。
- **震动**：`vibration_init()` + 合并回调里 `vibration_trigger()`（约 100ms）。
- **NVS**：`app_main` 调用 `nvs_flash_init()`；**game2048 组件内未实现 best_score 的 NVS 读写**（与参考项目 README 中「NVS 持久化最高分」描述不一致，以源码为准）。

### 运行时架构

```
app_main()
  ├─ nvs_flash_init()
  ├─ init_axp192_power()
  ├─ vibration_init()
  ├─ init_display()              // bsp_display_start_with_config + 背光 60%
  ├─ init_game_2048()
  ├─ init_ui_renderer()
  ├─ bsp_touch_new()
  ├─ init_game_input()
  ├─ show_startup_countdown()
  └─ xTaskCreate(game_loop_task) // 栈 8192，优先级 5

game_loop_task (循环 ~16ms)
  ├─ render_requested → ui_renderer_render_game()
  ├─ 检测 WON / GAME_OVER → 弹窗 + 启用长按重启
  └─ 结束态下轮询触摸 → 长按 2s → esp_restart()
```

输入侧在 `input_handler_init` 中启动识别器；移动成功后通过 `render_callback`（`request_game_render`）置位 `render_requested`。

---

## 未编入主程序的能力（仓库内保留）

`main/CMakeLists.txt` 已注释，主游戏不依赖：

| 模块 | 说明 |
|------|------|
| `shake_*` / `imu_mpu6886` | 摇一摇重启，已改为长按软复位 |
| `sk6812_led` | RGB 灯效已移除 |
| `joystick2_test.cpp` 等 | M5 Joystick2 I2C 测试 |
| `ili9342c_esp32` | 自研显示驱动，实际走 BSP + `esp_lcd_ili9341` |

---

## 游戏逻辑分层

```
game_2048 (对外 API)
    └── game_logic (移动、随机块、胜负判定、合并回调)
            └── game_state (棋盘、分数、best_score、状态枚举)
```

**移动流程（简化）：**

1. `game_2048_move_*` → `game_logic_move(direction)`  
2. 若棋盘有变化：合并计分 → 触发 `merge_callback` → `game_logic_add_random_tile()`  
3. 更新 `can_move` / 检测是否达到 `win_value` 或无路可走  

**`game_state_reset`：** 清空棋盘与当前分，**不重置** `best_score`。

**配置默认值（`game_config.h`）：**

| 字段 | 默认值 |
|------|--------|
| `win_value` | 2048 |
| `max_value` | 65535 |
| `tile_2_probability` | 100% |
| `tile_4_probability` | 0% |

---

## API 参考

### 游戏状态（`game2048/game/game_state.h`）

```c
#define GAME_BOARD_SIZE 4

typedef enum {
    GAME_STATE_PLAYING = 0,
    GAME_STATE_WON,
    GAME_STATE_GAME_OVER
} game_state_t;

typedef enum {
    MOVE_UP = 0, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT
} move_direction_t;

typedef struct {
    uint16_t board[GAME_BOARD_SIZE][GAME_BOARD_SIZE];
    uint32_t score;
    uint32_t best_score;
    game_state_t state;
    bool has_moved;
    bool can_move;
} game_state_data_t;
```

### 游戏核心（`game2048/game/game_2048.h`）

```c
int  game_2048_create(const game_config_t* config, game_2048_handle_t* handle_out);
int  game_2048_init(game_2048_handle_t handle);
int  game_2048_reset(game_2048_handle_t handle);
int  game_2048_destroy(game_2048_handle_t handle);

bool game_2048_move_left/right/up/down(game_2048_handle_t handle);
bool game_2048_is_game_over(game_2048_handle_t handle);
bool game_2048_has_won(game_2048_handle_t handle);
bool game_2048_can_move(game_2048_handle_t handle);

const game_state_data_t* game_2048_get_state(game_2048_handle_t handle);
int  game_2048_set_state(game_2048_handle_t handle, game_state_t state);

typedef void (*merge_callback_t)(int score_gained, void* user_data);
int  game_2048_set_merge_callback(game_2048_handle_t handle,
                                  merge_callback_t callback, void* user_data);
```

### 游戏逻辑（内部，`game2048/game/game_logic.h`）

```c
int game_logic_move(game_logic_handle_t handle, move_direction_t direction);
int game_logic_add_random_tile(game_logic_handle_t handle);
int game_logic_add_initial_tiles(game_logic_handle_t handle);
int game_logic_set_merge_callback(...);
int game_logic_get_merge_callback(...);
```

### UI 渲染（`game2048/ui/ui_renderer.h`）

```c
int ui_renderer_create(lv_obj_t* screen, const ui_config_t* config,
                       ui_renderer_handle_t* handle_out);
int ui_renderer_init(ui_renderer_handle_t handle);

int ui_renderer_render_game(ui_renderer_handle_t handle,
                            const game_state_data_t* game_state);        // 局部刷新
int ui_renderer_render_game_full(ui_renderer_handle_t handle,
                                 const game_state_data_t* game_state);
int ui_renderer_render_board / _partial / _score / _status(...);

int ui_renderer_show_win_dialog(ui_renderer_handle_t handle,
                                uint32_t score, uint32_t best_score);
int ui_renderer_show_gameover_dialog(ui_renderer_handle_t handle,
                                     uint32_t score, uint32_t best_score);
int ui_renderer_hide_dialog(ui_renderer_handle_t handle);
bool ui_renderer_is_dialog_visible(ui_renderer_handle_t handle);
int ui_renderer_destroy(ui_renderer_handle_t handle);
```

缓存结构 `ui_render_cache_t` 保存上一帧棋盘与分数，用于差分更新。

### 输入处理（`game2048/input/input_handler.h`）

```c
typedef struct {
    void (*render_callback)(void);
    ui_renderer_handle_t ui_handle;
} input_handler_config_t;

int input_handler_create(game_2048_handle_t game_handle,
                         esp_lcd_touch_handle_t touch_handle,
                         const input_handler_config_t* config,
                         input_handler_handle_t* handle_out);
int input_handler_init(input_handler_handle_t handle);
void input_handler_stop(input_handler_handle_t handle);
int input_handler_destroy(input_handler_handle_t handle);
```

弹窗可见时，input_handler **优先处理关闭/重置**，不再执行棋盘移动。

### 震动（`vibration/vibration.h`）

```c
esp_err_t vibration_init(void);
esp_err_t vibration_trigger(void);                              // 默认 ~100ms
esp_err_t vibration_start(uint8_t strength, uint16_t duration_ms);
esp_err_t vibration_stop(void);
esp_err_t vibration_deinit(void);
```

### 主程序关键集成点（`main/main.c`）

```c
// 合并回调 → 震动
static void on_tile_merge(int score_gained, void* user_data);

// 渲染请求（由 input_handler 的 render_callback 触发）
static void request_game_render(void);

// 游戏循环任务：渲染、胜负 UI、长按重启
static void game_loop_task(void *pvParameters);

// 长按重启（仅 WON / GAME_OVER，2000ms）
static const uint32_t LONG_PRESS_DURATION_MS = 2000;
```

显示初始化示例：

```c
bsp_display_cfg_t cfg = {
    .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
    .buffer_size = 320 * 40,
    .double_buffer = true,
    .flags = { .buff_dma = true, .buff_spiram = false },
};
display = bsp_display_start_with_config(&cfg);
bsp_display_brightness_set(60);
```

---

## 主程序构建依赖（`main/CMakeLists.txt`）

当前 `SRCS`：`main.c`, `font_8x8.c`, `font_renderer.c`, `touch_direction.c`, `touch_direction_recognizer.cpp`

`PRIV_REQUIRES`：`espressif__esp_lcd_touch_ft5x06`, `ili9342c_esp32`, `game2048`, `vibration`, `axp192_esp32`

---

## 性能与资源（参考项目 README / 日志）

| 指标 | 约值 |
|------|------|
| 局部刷新 | ~50ms/步 |
| 全屏重绘 | ~200ms |
| 游戏循环 | ~16ms 周期，按需渲染 |
| 触摸采样 | 识别器侧约 100Hz 量级 |
| `game_loop` 任务栈 | 8192 字节 |

---

## 文档与源码差异（调研时注意）

1. **NVS 最高分**：README 声称 NVS 持久化 `best_score`；源码仅在 `app_main` 初始化 NVS，`game_state` 仅在 RAM 维护最高分。若需掉电保存，需在 ChroneCore 或移植层自行实现。
2. **长按重启**：README 写「任意时刻长按」；`main.c` 仅在 `long_press_reset_enabled`（胜负态）且 `game_loop_task` 内检测，非全局长按。
3. **目标芯片**：README 构建说明含 `esp32s3`；硬件为 Core2（ESP32）。以实际 `sdkconfig` / `set-target` 为准。

---

## 对 ChroneCore 的可复用点

| 能力 | 参考位置 | 说明 |
|------|----------|------|
| BSP + LVGL 显示管线 | `main.c` → `init_display()` | 双缓冲、DMA、背光 |
| 触摸 BSP | `bsp_touch_new` | FT5x06 |
| AXP192 供电 | `components/axp192_esp32` | 旧版 I2C，注意与 BSP 总线冲突 |
| 事件驱动 UI 刷新 | `render_requested` + `game_loop_task` | 避免在触摸回调里直接重绘 |
| 分层组件 | `game` / `ui` / `input` 独立组件 | 便于闹钟多「应用」切换 |
| 弹窗与手势分流 | `input_handler.cpp` | 模态 UI 与游戏输入分离 |
| 震动反馈 | `vibration` + merge callback | 可扩展到闹钟提醒 |

参考项目 **未实现**、但 README 规划的功能：音效、合并动画、多棋盘尺寸、撤销、统计、主题、WiFi 排行榜等。

---

## 构建参考项目（2048_M5Stack_Core2）

```bash
# Windows 快捷方式
build_esp32.bat

# 或手动（在参考项目目录）
idf.py set-target esp32   # 以项目实际 target 为准
idf.py build
idf.py -p COMx flash monitor
```

环境要求：ESP-IDF 5.5+、Python 3.8+、M5Stack Core2 硬件。

---

## ChroneCore 后续

- [ ] 确定 ESP-IDF target 与分区表  
- [ ] 从参考项目抽取 BSP/LVGL/触摸初始化模板  
- [ ] 定义多界面状态机（时钟 / 天气 / 秒表 / …）  
- [ ] 按需实现 NVS 配置与最高分/闹钟持久化  

---

## 许可

ChroneCore 仓库许可见 [LICENSE](LICENSE)。参考项目 2048_M5Stack_Core2 为 MIT License。
