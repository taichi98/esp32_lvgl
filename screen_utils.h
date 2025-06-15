#pragma once
#include <lvgl.h>

static inline void lv_load_and_delete(lv_obj_t *new_screen)
{
    lv_obj_t *old_screen = lv_scr_act();
    lv_scr_load(new_screen);
    if (old_screen && old_screen != new_screen) {
        lv_obj_del(old_screen);
    }
}
