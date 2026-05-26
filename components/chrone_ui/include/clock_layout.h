#pragma once

/** M5Stack Core2 landscape layout (320x240), see docs/clock-ui-reference.md */

#define CHRONE_LCD_W           320
#define CHRONE_LCD_H           240

#define CHRONE_HEADER_FONT_SIZE  22
#define CHRONE_HEADER_H            32
#define CHRONE_FOOTER_H            20

/* DSEG7 bitmap font (see components/chrone_ui/fonts/) */
#define CHRONE_TIME_FONT_SIZE    56
#define CHRONE_TIME_FONT_LINE_H  57

#define CHRONE_TIME_BLOCK_H      CHRONE_TIME_FONT_LINE_H
#define CHRONE_TIME_Y            (CHRONE_HEADER_H + ((CHRONE_LCD_H - CHRONE_HEADER_H - CHRONE_FOOTER_H - CHRONE_TIME_BLOCK_H) / 2))

/* Analog dial (DS3231_Clock style, 320×240 landscape) */
#define CHRONE_ANALOG_RADIUS       86
#define CHRONE_ANALOG_PAD          12
#define CHRONE_ANALOG_CANVAS_SZ    (2 * (CHRONE_ANALOG_RADIUS + CHRONE_ANALOG_PAD))
#define CHRONE_ANALOG_CX           (CHRONE_LCD_W / 2)
#define CHRONE_ANALOG_CY           (CHRONE_HEADER_H + 8 + CHRONE_ANALOG_RADIUS)
