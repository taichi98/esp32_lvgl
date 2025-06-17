#ifndef BRIGHTNESS_MANAGER_H
#define BRIGHTNESS_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>

extern lv_obj_t *brightness_screen;

// Khởi tạo module điều chỉnh độ sáng
void brightness_manager_init(int pin_bl, int initial_percent = 50);

// Đọc độ sáng đã lưu từ EEPROM
int brightness_load_from_eeprom(int default_percent);

// Sự kiện khi trượt thanh chỉnh sáng
void brightness_slider_event_cb(lv_event_t * e);

// Hàm tạo màn hình điều chỉnh độ sáng (chỉ tạo 1 lần, dùng lại)
lv_obj_t *create_brightness_screen();

#endif // BRIGHTNESS_MANAGER_H
