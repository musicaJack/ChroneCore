#include "chrone_ui_layout.h"

void chrone_ui_no_scroll(lv_obj_t *obj)
{
    if (!obj) {
        return;
    }
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    /* 防止子控件拖动时把滑动传给父级/屏幕 */
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN);
}

void chrone_ui_prepare_screen(lv_obj_t *screen)
{
    chrone_ui_no_scroll(screen);
}

lv_obj_t *chrone_ui_cont_create(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    chrone_ui_no_scroll(obj);
    return obj;
}
