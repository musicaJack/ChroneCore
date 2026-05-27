# Chrono OS - 闹钟设置开发建议文档
（基于 M5Stack Core2 For AWS）

Version: v1.0
Target Hardware: M5Stack Core2 For AWS
Platform: ESP32 + LVGL
Author: ChatGPT System Design Reference

---

# 一、项目目标

本项目目标不是单纯开发“电子闹钟”。

而是：

打造一款具有 Apple 风格体验的：
# 「时间交互设备（Time Interaction Device）」

设备应具备：

- 安静
- 克制
- 高级
- 柔和
- 可信赖
- 半睡眠状态下易操作

的交互体验。

核心设计参考：
- iPhone Clock
- Apple Watch Alarm
- Braun Clock
- 日本床头电子钟

---

# 二、硬件能力分析（Core2 For AWS）

Core2 For AWS 硬件资源非常适合高级闹钟应用。

## 已具备的重要硬件

| 硬件 | 用途 |
|---|---|
| 2.0 IPS Touch LCD | UI交互 |
| 电容触摸 | 手势/滚轮 |
| 内置震动马达 | Haptic反馈 |
| Speaker + I2S AMP | Tick音/闹钟 |
| RTC BM8563 | 时间保持 |
| IMU MPU6886/BMI270 | 抬手亮屏 |
| ESP32 双核 | UI + 后台任务 |
| 500mAh Battery | 床头离线使用 |
| WiFi | NTP同步 |
| 麦克风 | 未来语音扩展 |

官方规格参考：
- 320x240 IPS
- ESP32 Dual Core 240MHz
- 16MB Flash
- 8MB PSRAM
- AXP192 PMU
- 内置 vibration motor

---

# 三、产品设计哲学（极重要）

Apple Clock 的核心：
# 不是功能。
而是：
# “低认知负担”。

用户通常在：
- 刚醒
- 半睡眠
- 夜间
- 精神疲劳

状态下使用闹钟。

因此：

## UI设计原则

必须：

- 一眼理解
- 一步完成
- 不需要思考
- 不产生焦虑

---

# 四、交互设计原则

---

# 1. 时间不是数字

Apple 从不把时间当成：
- 参数输入

而是：
# “物理对象”。

所以：

时间应该：
- 可拨动
- 有惯性
- 有卡位
- 有阻尼感

---

# 2. 每一步必须有反馈

用户：
- 滑动
- 点击
- 切换
- 停靠

系统都必须回应。

反馈包括：

| 类型 | 实现 |
|---|---|
| 视觉 | 动画 |
| 触觉 | vibration |
| 听觉 | tick |
| 运动 | 惯性 |

原则：
# 用户永远不能怀疑“是否操作成功”。

---

# 五、推荐UI结构

---

# 闹钟设置主界面

建议布局：

```text
22:48

Wednesday
May 27

Alarm 07:00 ON

Next alarm in 8h12m