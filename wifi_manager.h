#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <lvgl.h>

// Định nghĩa struct lưu thông tin WiFi
struct WiFiCredential {
    String ssid;
    String password;
};

// Biến hiển thị trạng thái WiFi trên giao diện chính
extern lv_obj_t *wifi_icon;
extern lv_obj_t *wifi_label;
extern lv_obj_t *wifi_screen;

// Cập nhật biểu tượng và trạng thái WiFi UI
void update_wifi_status_ui();

// Hàm khởi tạo và trả về màn hình WiFi (theo screen manager)
lv_obj_t *create_wifi_screen();

// Tải danh sách mạng WiFi đã lưu từ SPIFFS
bool loadWiFiCredentials(std::vector<WiFiCredential> &wifiList);

// Lưu danh sách mạng WiFi vào SPIFFS
void saveWiFiCredentials(const std::vector<WiFiCredential> &wifiList);

// Thêm hoặc cập nhật thông tin mạng WiFi đã lưu
void addWiFiNetwork(const String &newSSID, const String &newPassword);

// Tạo task tự động giám sát và kết nối WiFi (chạy nền)
void wifi_monitor_task(void *param);

#endif // WIFI_MANAGER_H
