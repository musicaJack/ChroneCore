#pragma once

/** M5Stack Core2 landscape layout (320x240), see docs/clock-ui-reference.md */

#define CHRONE_LCD_W           320
#define CHRONE_LCD_H           240

#define CHRONE_HEADER_FONT_SIZE  22
#define CHRONE_HEADER_H            36
#define CHRONE_FOOTER_H            32
/** 底部三区虚拟键高度（与 Core2-for-AWS Button_Attach h=60 一致） */
#define CHRONE_VIRTUAL_BTN_H         60
#define CHRONE_WEATHER_ICON_SZ     24

/* DSEG7 bitmap font (see components/chrone_ui/fonts/) */
#define CHRONE_TIME_FONT_SIZE    56
#define CHRONE_TIME_FONT_LINE_H  57

#define CHRONE_TIME_BLOCK_H      CHRONE_TIME_FONT_LINE_H
#define CHRONE_TIME_Y            (CHRONE_HEADER_H + ((CHRONE_LCD_H - CHRONE_HEADER_H - CHRONE_FOOTER_H - CHRONE_TIME_BLOCK_H) / 2))

/** 闹钟页顶栏（返回键热区，需保持最前层） */
#define CHRONE_ALARM_HEADER_H      42
#define CHRONE_ALARM_NAV_BTN_W     88
#define CHRONE_ALARM_NAV_BTN_H     40

/** 闹钟编辑页：DSEG 滚轮与控件分区 */
#define CHRONE_ALARM_PICKER_Y      (CHRONE_ALARM_HEADER_H + 4)
#define CHRONE_ALARM_PICKER_H      CHRONE_TIME_FONT_LINE_H
#define CHRONE_ALARM_META_Y        (CHRONE_ALARM_PICKER_Y + CHRONE_ALARM_PICKER_H + 10)

/* Analog dial (DS3231_Clock style, 320×240 landscape) */
#define CHRONE_ANALOG_RADIUS       86
#define CHRONE_ANALOG_PAD          12
#define CHRONE_ANALOG_CANVAS_SZ    (2 * (CHRONE_ANALOG_RADIUS + CHRONE_ANALOG_PAD))
#define CHRONE_ANALOG_CX           (CHRONE_LCD_W / 2)
#define CHRONE_ANALOG_CY           (CHRONE_HEADER_H + 8 + CHRONE_ANALOG_RADIUS)
