#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Các ID cho từng màn hình
typedef enum {
    SCREEN_NONE = -1,
    SCREEN_MAIN,
    SCREEN_SETTING,
    SCREEN_BRIGHTNESS,
    SCREEN_WIFI,
    SCREEN_CALIBRATION,
    SCREEN_TOTAL  // Tổng số màn hình
} ScreenID;

void screen_manager_init();                  // Khởi tạo hệ thống quản lý màn hình
void switch_to_screen(ScreenID id);          // Chuyển đến màn hình được chỉ định
void cleanup_screens();                      // Xoá và huỷ toàn bộ màn hình
lv_obj_t* get_screen(ScreenID id);           // Truy cập trực tiếp màn hình nếu cần

#ifdef __cplusplus
}
#endif

#endif  // SCREEN_MANAGER_H
