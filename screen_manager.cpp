#include "screen_manager.h"
#include "brightness_manager.h"
#include "wifi_manager.h"
#include "calibrate_touch.h"
/* ===============================================================================================

Với module này, các màn hình khi được gọi sẽ được khởi tạo đúng 1 lần, và lưu trong heap/
Sau đó sẽ được tái sử dụng.
Nhưng nó sẽ vẫn chiếm 1 lượng lớn bộ nhớ heap. Dù sẽ load màn hình nhanh hơn./
Khi thoát hẳn 1 màn hình lớn (nhiều đối tượng) dùng lệnh cleanup_screens() để dọn sạch màn hình

=============================================================================================== */
// extern static lv_obj_t *wifi_screen = nullptr;
// extern static lv_obj_t * brightness_screen = nullptr;
// Forward declarations (khai báo trước)
extern lv_obj_t* create_main_screen();
extern lv_obj_t* create_setting_screen();
extern lv_obj_t* create_brightness_screen();
extern lv_obj_t* create_wifi_screen();
extern lv_obj_t* create_calibration_screen();

// Mảng chứa tất cả màn hình
static lv_obj_t* screen_registry[SCREEN_TOTAL] = { NULL };
static ScreenID current_screen = SCREEN_NONE;

void screen_manager_init() {
    for (int i = 0; i < SCREEN_TOTAL; ++i) {
        screen_registry[i] = NULL;
    }
}

lv_obj_t* get_screen(ScreenID id) {
    if (id < 0 || id >= SCREEN_TOTAL) return NULL;
    return screen_registry[id];
}

void switch_to_screen(ScreenID id) {
    if (id < 0 || id >= SCREEN_TOTAL) return;
    if (id == current_screen && screen_registry[id]) {
        lv_scr_load(screen_registry[id]);  // Nếu đang ở màn này, chỉ reload
        return;
    }

    // Nếu màn hình chưa khởi tạo -> khởi tạo
    if (!screen_registry[id]) {
        switch (id) {
            case SCREEN_MAIN:
                screen_registry[id] = create_main_screen();
                break;
            case SCREEN_SETTING:
                screen_registry[id] = create_setting_screen();
                break;
            case SCREEN_BRIGHTNESS:
                screen_registry[id] = create_brightness_screen();
                break;
            case SCREEN_WIFI:
                screen_registry[id] = create_wifi_screen();
                break;
            case SCREEN_CALIBRATION:
                screen_registry[id] = create_calibration_screen();
                break;
            default:
                break;
        }
    }

    // Tải màn hình đã được khởi tạo
    if (screen_registry[id]) {
        lv_scr_load(screen_registry[id]);
        current_screen = id;
    }
}

void cleanup_screens() {
    for (int i = 0; i < SCREEN_TOTAL; ++i) {
        if (screen_registry[i]) {
            lv_obj_del(screen_registry[i]);
            screen_registry[i] = NULL;
        }
    }
    wifi_screen = NULL;
    brightness_screen = NULL;  
    current_screen = SCREEN_NONE;
}
