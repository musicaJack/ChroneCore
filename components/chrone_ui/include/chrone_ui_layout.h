#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 禁止对象整体滑动（LVGL 默认 container 可 scroll） */
void chrone_ui_no_scroll(lv_obj_t *obj);

/** 根 screen（与 chrone_ui_no_scroll 相同，进入任意全屏页时调用） */
void chrone_ui_prepare_screen(lv_obj_t *screen);

/** 创建透明容器并默认禁止滚动 */
lv_obj_t *chrone_ui_cont_create(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif
