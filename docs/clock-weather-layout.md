# 时钟屏天气与信息条布局方案（草案）

**屏幕：** M5Stack Core2 横屏 320×240 · **语言：** 界面仅 **英文 + 图标**（不用中文）  
**原则：** 不移动数字时间 Y、不缩小模拟表盘；数字/模拟共用同一顶栏 + 底栏。

参考实现：`HourChime`（`app/weather_icons/`、`lcd_display.cc` `SetWeatherInfo`）。

---

## 1. 数据与显示内容

| 数据 | 来源 | 界面显示 |
|------|------|----------|
| 日期 | `struct tm` | `YYYY-MM-DD` |
| 星期 | `chrone_time_weekday_en()` | `Sat` / `Saturday`（英文，与现有顶栏一致） |
| 城市 | `weather_info.city` 或 `weather_city_format_title_case` | `Shanghai`（拼音 Title Case，不用 `name_zh`） |
| 现象 | `weather_info.text` | 英文：`Cloudy`、`Rainy-1`、`Strong Wind`（来自 `weather_code_get_desc` + `language=en`） |
| 温度 | `weather_info.temp` | `25°C` |
| 图标 | `weather_info.code` + 昼夜 | 24×24 RGB565（由 HourChime 48×48 1-bit 缩放/转换） |

未联网 / 无数据：`--` 或隐藏图标；不显示中文占位。

---

## 2. 推荐布局：方案 A（顶栏日历 + 底栏天气带）

```
┌────────────────────────────── 320 ──────────────────────────────┐
│ 2026-05-26                              Saturday        │ 36px │ 顶栏
├───────────────────────────────────────────────────────────────┤
│                                                               │
│           [ 数字 HH:MM:SS  或  模拟表盘 196×196 ]              │ 主区
│              （CHRONE_TIME_Y / ANALOG_CY 不变）                 │
│                                                               │
├───────────────────────────────────────────────────────────────┤
│  [icon]  Cloudy   25°C                        Shanghai │32px│ 底栏
└───────────────────────────────────────────────────────────────┘
```

### 2.1 顶栏（`CHRONE_HEADER_H` = 36，保持）

| 控件 | 对齐 | 字体 | 内容 |
|------|------|------|------|
| `label_date` | 左上 (8, 4) | Montserrat 22 | `2026-05-26` |
| `label_week` | 右上 (-8, 4) | Montserrat 14 | `Saturday` |

**移除** 当前右上角的 `city + temp` 合并行（改到底栏）。

### 2.2 底栏（新建，`CHRONE_FOOTER_H`：20 → **32**）

横向 flex 容器，垂直居中，透明背景：

| 顺序 | 控件 | 尺寸 | 内容 |
|------|------|------|------|
| 1 | `lv_img` 天气图标 | **24×24** | `weather_icon_get_bitmap(code, is_day)` |
| 2 | `label_weather_text` | Montserrat 14 | `Cloudy` / `Rainy-1` |
| 3 | `label_temp` | Montserrat 14 | `25°C` |
| 4 | （弹性空白） | — | — |
| 5 | `label_city` | Montserrat 14 | `Shanghai` |

图标与文字间距 4px；整栏距底边约 4px。

### 2.3 主区标定（不改）

- 数字：`CHRONE_TIME_Y` 仍按「顶 36 + 底 32」居中公式（实现时更新 `clock_layout.h` 中 `FOOTER_H` 后重算一次 Y，若与现值差 ≤2px 可保持现 Y）。
- 模拟：`CHRONE_ANALOG_CY` / `RADIUS` 不变；表盘下边约 Y≈228，底栏 32px 贴在 208–240，不重叠。

### 2.4 刷新策略

- 日期/星期：1 Hz，仅变化时 invalidate（沿用 `update_header`）。
- 底栏天气：1 Hz 调 `update_weather_strip()`；`code` 变时重绘图标 + 文案。
- 图标昼夜：`is_day` = 本地时 06:00–18:00（与 HourChime 相同）。

---

## 3. 备选方案（未采用，供对比）

| 方案 | 概要 | 缺点 |
|------|------|------|
| **B** 顶栏两行 | 第二行放 city / text / temp / 小图标 | 顶栏增高，需下移主区 |
| **C** 仅拆顶右 | 右上多 label，底栏不用 | 320 宽右侧仍挤，难放 24px 图标 |

**结论：** 采用 **方案 A**。

---

## 4. 实现依赖（见 `docs/TODO.md`）

1. 自 HourChime 移植 `weather_icon_map` + `icons_48x48` + `weather_1bit_to_rgb565`。
2. `chrone_ui`：顶栏去掉合并天气 label；增加底栏 flex + 图标/现象/温度/城市。
3. 保持 API `language=en`；UI 只绑 `weather_info.text`（已是英文描述）。
4. 星期显示统一走 `chrone_time_weekday_en`（顶栏不再用中文星期）。

---

## 5. 与 HourChime 差异

| 项目 | HourChime | ChroneCore（本方案） |
|------|-----------|----------------------|
| 城市位置 | 状态栏中间 | 底栏右 |
| 图标尺寸 | 48×48 | **24×24**（横屏高度限制） |
| 现象文字 | 不显示 | **显示** `weather_info.text`（英文） |
| 界面语言 | 混用 | **仅英文 + 图标** |
