#include "style_msgbox.h"
#include "brightness_16.h"
#include "wifi_manager.h"

extern void lv_create_main_gui(void);
extern void lv_setting_box(void);
extern void lv_open_brightness_screen(int default_percent);
extern int saved_brightness;

static lv_style_t icon_style;
static lv_style_t style_pressed;

void init_styles(void) {
  lv_style_init(&icon_style);
  lv_style_set_text_font(&icon_style, &lv_brightness_16);

  // Style khi nhấn (pressed)
  lv_style_init(&style_pressed);
  lv_style_set_bg_color(&style_pressed, lv_color_hex(0xE0E0E0)); // Màu xám nhạt
  lv_style_set_bg_opa(&style_pressed, LV_OPA_COVER);
  lv_style_set_transform_zoom(&style_pressed, 255); // Phóng nhẹ
}

void style_msgbox_customize_header(lv_obj_t * msgbox) {
    // Lấy phần header từ msgbox
    lv_obj_t * header = lv_msgbox_get_header(msgbox);
    if (!header) return;

    // Giảm padding để làm header nhỏ hơn
    lv_obj_set_style_pad_top(header, 1, 0);
    lv_obj_set_style_pad_bottom(header, 1, 0);
    lv_obj_set_style_pad_left(header, 8, 0);
    lv_obj_set_style_pad_right(header, 8, 0);
    // lv_obj_set_style_min_height(header, 20, 0);  // Có thể giảm thêm nếu muốn

    // Tùy chọn: Giảm cỡ font tiêu đề nếu cần
    lv_obj_t * label = lv_obj_get_child(header, 0);
    if (label) {
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    }

    // Duyệt các nút để tùy chỉnh kích thước
    uint32_t count = lv_obj_get_child_cnt(header);
    if (count >= 2) {
        lv_obj_t * btn_close = lv_obj_get_child(header, count - 1);
        lv_obj_set_size(btn_close, 24, 24);
        lv_obj_set_style_radius(btn_close, 12, 0);
        lv_obj_set_style_pad_all(btn_close, 1, 0);

        lv_obj_t * icon = lv_obj_get_child(btn_close, 0);
        if (icon) {
            lv_obj_set_style_text_font(icon, &lv_font_montserrat_12, 0);
        }
    }
}

// Hàm tạo hiệu ứng khi nhấn nút.
static void btn_effect_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = (lv_obj_t *)lv_event_get_target(e);

    if(code == LV_EVENT_PRESSED) {
        lv_obj_set_style_opa(target, LV_OPA_50, 0);
        lv_obj_set_style_transform_scale(target, 240, 0); // 240/255 = ~0.94x
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        lv_obj_set_style_opa(target, LV_OPA_COVER, 0);
        lv_obj_set_style_transform_scale(target, 255, 0);
    }
}

lv_obj_t * create_icon_button(const void * img_src, const char * label_text,
                              lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,
                              lv_event_cb_t event_cb_clicked) {
    // Tạo container chứa icon và label
    lv_obj_t * cont = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 85, 85);
    lv_obj_align(cont, align, x_ofs, y_ofs);

    // Tạo icon
    lv_obj_t * icon = lv_img_create(cont);
    lv_img_set_src(icon, img_src);
    lv_obj_set_size(icon, 64, 64);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 0);

    // Tạo label
    lv_obj_t * label = lv_label_create(cont);
    lv_label_set_text(label, label_text);
    lv_obj_align_to(label, icon, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);

    // Thêm hiệu ứng và sự kiện click
    lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cont, btn_effect_event_cb, LV_EVENT_ALL, NULL);
    if(event_cb_clicked) {
        lv_obj_add_event_cb(cont, event_cb_clicked, LV_EVENT_CLICKED, NULL);
    }

    return cont;
}

lv_obj_t * create_setting_item(lv_obj_t * parent, const char * icon_text, const char * label_text, lv_event_cb_t event_cb, const lv_font_t * icon_font) {
    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, lv_pct(100), 40);
    lv_obj_set_style_pad_all(cont, 10, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_width(cont, 1, 0);
    lv_obj_set_style_border_color(cont, lv_color_hex(0xF2F2F2), 0);
    lv_obj_set_style_border_side(cont, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_add_style(cont, &style_pressed, LV_STATE_PRESSED);

    lv_obj_t * icon = lv_label_create(cont);
    lv_label_set_text(icon, icon_text);
    lv_obj_set_style_pad_right(icon, 8, 0);
    if (icon_font) {
        lv_obj_set_style_text_font(icon, icon_font, 0);  // Áp dụng font nếu có
    }

    lv_obj_t * label = lv_label_create(cont);
    lv_label_set_text(label, label_text);

    lv_obj_add_event_cb(cont, event_cb, LV_EVENT_CLICKED, NULL);

    return cont;
}

static inline void lv_load_and_delete(lv_obj_t *new_screen)
{
    lv_obj_t *old_screen = lv_scr_act();
    lv_scr_load(new_screen);
    if (old_screen && old_screen != new_screen) {
        lv_async_call([](void *scr){ lv_obj_del((lv_obj_t *)scr); },
                      old_screen);
    }
}

static void switch_screen_cb(void *user_data)
{
    screen_id_t id = static_cast<screen_id_t>((uintptr_t)user_data);

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_load_and_delete(scr);

    switch(id) {
        case SCREEN_MAIN:
            lv_create_main_gui();
            break;
        case SCREEN_SETTING:
            lv_setting_box();
            break;
        case SCREEN_WIFI_SCAN:
            create_wifi_scan_screen();
            break;
        case SCREEN_BRIGHTNESS:
            lv_open_brightness_screen(saved_brightness);
            break;
        default:
            break;
    }
}

void lv_switch_screen(screen_id_t id)
{
    lv_async_call(switch_screen_cb, (void *)(uintptr_t)id);
}

