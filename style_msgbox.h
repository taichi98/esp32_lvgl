#ifndef STYLE_MSGBOX_H
#define STYLE_MSGBOX_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_font_t lv_brightness_16;

void init_styles(void);

void style_msgbox_customize_header(lv_obj_t * msgbox);

lv_obj_t * create_icon_button(const void * img_src, const char * label_text,
                              lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,
                              lv_event_cb_t event_cb_clicked);

lv_obj_t * create_setting_item(lv_obj_t * parent, const char * icon_text, const char * label_text, lv_event_cb_t event_cb, const lv_font_t * icon_font = NULL);

typedef enum {
    SCREEN_MAIN,
    SCREEN_SETTING,
    SCREEN_WIFI_SCAN,
    SCREEN_BRIGHTNESS
} screen_id_t;

void lv_switch_screen(screen_id_t id);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // STYLE_MSGBOX_H
