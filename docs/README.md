# ChroneCore 项目文档

本目录为 **ChroneCore**（M5Stack Core2 多功能智能闹钟）的设计与开发文档。

| 文档 | 说明 |
|------|------|
| [requirements.md](requirements.md) | 产品需求、功能范围、非功能需求 |
| [api-reference.md](api-reference.md) | 硬件 BSP、业务模块、外部 API（心知天气）、参考仓库 API 对照 |
| [clock-weather-layout.md](clock-weather-layout.md) | 时钟屏顶/底栏布局（英文 + 图标，方案 A） |
| [alarm-implementation.md](alarm-implementation.md) | 阶段 4 闹钟：NVS、左+右进配置、触摸/摇一摇停止 |
| [clock-ui-reference.md](clock-ui-reference.md) | **数字/模拟时钟 UI**：NanoTimer、DS3231_Clock 样式与移植规格 |
| [architecture.md](architecture.md) | 系统架构、模块划分、数据流、状态机、存储 |
| [development-plan.md](development-plan.md) | 分阶段开发计划、里程碑、风险与验收 |
| [TODO.md](TODO.md) | **当前开发待办清单**（随进度更新） |

## 参考工程

| 路径 | 角色 |
|------|------|
| `C:/Users/sh_mu/code/2048_M5Stack_Core2` | **主基准**：ESP-IDF 5.5+、M5Stack BSP、LVGL 8、AXP192、触摸、工程结构 |
| `C:/Users/sh_mu/code/Core2-for-AWS-IoT-Kit` | **硬件 API**：SK6812、MPU6886、BM8563、麦克风/扬声器、TF 卡 |
| `C:/Users/sh_mu/code/HourChime` | **联网与天气**：WiFi 热点配网、配网页城市选择、心知天气 API |
| `C:/Users/sh_mu/code/NanoTimer` | **数字时钟 UI**：七段 `HH:MM:SS`、顶栏日期/星期、秒表逻辑 |
| `C:/Users/sh_mu/code/DS3231_Clock` | **模拟时钟 UI**：表盘刻度、三针、配色、秒级局部刷新 |

## 文档版本

- 初版：2026-05-26
