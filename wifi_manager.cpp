#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "wifi_manager.h"
#include "style_msgbox.h"
#include "resources.h"
#include "screen_manager.h"

// Biến toàn cục
lv_obj_t *wifi_icon = nullptr;
lv_obj_t *wifi_label = nullptr;


struct WifiUIElements {
    lv_obj_t *wifi_list = NULL;
    lv_obj_t *scanning_label = NULL;
    lv_obj_t *mbox = NULL;
    lv_obj_t *kb = NULL;
    lv_obj_t *btn_back = NULL;
    lv_obj_t *btn_refresh = NULL;
} wifiUI;

struct WifiConnectData {
    lv_obj_t *mbox;
    lv_obj_t *ta;
    lv_obj_t *kb;
    String ssid;
};

lv_obj_t *wifi_screen = nullptr;

static void check_scan_result(lv_timer_t *timer);
void addWiFiNetwork(const String &newSSID, const String &newPassword);

void update_wifi_status_ui() {
    if (!wifi_icon || !wifi_label) return;
    if (!lv_obj_is_valid(wifi_icon) || !lv_obj_is_valid(wifi_label)) return;

    if (WiFi.status() == WL_CONNECTED) {
        lv_img_set_src(wifi_icon, &wifi_connected);
        String ssid = WiFi.SSID();
        lv_label_set_text_fmt(wifi_label, ssid.length() > 0 ? "%s" : "WiFi OK", ssid.c_str());
        lv_obj_set_width(wifi_label, 60);
        lv_label_set_long_mode(wifi_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_align_to(wifi_label, wifi_icon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    } else {
        lv_img_set_src(wifi_icon, &no_wifi);
        lv_label_set_text(wifi_label, "No wifi");
    }
}

static void ta_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *kb = (lv_obj_t *)lv_event_get_user_data(e);
    if (code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(kb);
        lv_obj_align(wifiUI.mbox, LV_ALIGN_CENTER, 0, -60);
    } else if (code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(wifiUI.mbox, LV_ALIGN_CENTER, 0, 0);
    }
}

static void wifi_network_selected(lv_event_t *e) {
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    const char *text = lv_list_get_btn_text(wifiUI.wifi_list, btn);
    wifiUI.kb = lv_keyboard_create(lv_layer_top());
    lv_obj_add_flag(wifiUI.kb, LV_OBJ_FLAG_HIDDEN);

    String display_text(text);
    int paren_index = display_text.indexOf('(');
    String ssid = display_text.substring(0, paren_index - 1);

    wifiUI.mbox = lv_msgbox_create(NULL);
    lv_obj_set_size(wifiUI.mbox, 300, 115);
    lv_obj_center(wifiUI.mbox);
    lv_msgbox_add_title(wifiUI.mbox, "Connect to WiFi");

    lv_obj_t *content = lv_msgbox_get_content(wifiUI.mbox);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *ssid_label = lv_label_create(content);
    lv_label_set_text(ssid_label, ssid.c_str());
    lv_obj_set_style_text_font(ssid_label, &lv_font_montserrat_16, 0);

    lv_obj_t *ta = lv_textarea_create(content);
    lv_textarea_set_password_mode(ta, true);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, "Enter password");
    lv_obj_set_width(ta, lv_pct(90));
    lv_obj_add_state(ta, LV_STATE_FOCUSED);

    WifiConnectData *data = new WifiConnectData{wifiUI.mbox, ta, nullptr, ssid};
    lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_ALL, wifiUI.kb);

    lv_obj_add_event_cb(ta, [](lv_event_t *e) {
        WifiConnectData *data = (WifiConnectData *)lv_event_get_user_data(e);
        String password = lv_textarea_get_text(data->ta);

        lv_obj_t *content = lv_msgbox_get_content(wifiUI.mbox);
        lv_obj_clean(content);
        lv_obj_align(wifiUI.mbox, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_flag(wifiUI.kb, LV_OBJ_FLAG_HIDDEN);

        lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_t *label = lv_label_create(content);
        lv_label_set_text_fmt(label, "Connecting to:\n%s", data->ssid.c_str());
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(label);

        WiFi.disconnect();
        delay(100);
        WiFi.begin(data->ssid.c_str(), password.c_str());
        WiFiCredential *connInfo = new WiFiCredential{data->ssid, password};

        lv_timer_create([](lv_timer_t *t) {
            static int retry = 0;
            WiFiCredential *info = (WiFiCredential *)t->user_data;

            if (WiFi.status() == WL_CONNECTED) {
                lv_obj_t *content = lv_msgbox_get_content(wifiUI.mbox);
                lv_obj_clean(content);
                lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
                lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
                lv_label_set_text(lv_label_create(content), "Connected successfully!");
                lv_obj_add_flag(wifiUI.kb, LV_OBJ_FLAG_HIDDEN);
                addWiFiNetwork(info->ssid, info->password);
                lv_timer_del(t);
                delete info;
            } else if (retry++ > 10) {
                lv_obj_t *content = lv_msgbox_get_content(wifiUI.mbox);
                lv_obj_clean(content);
                lv_label_set_text(lv_label_create(content), "Connection failed.");
                lv_obj_add_flag(wifiUI.kb, LV_OBJ_FLAG_HIDDEN);
                lv_timer_del(t);
                delete info;
            }
        }, 1000, connInfo);

        delete data;
    }, LV_EVENT_READY, data);

    lv_obj_t *close_btn = lv_msgbox_add_close_button(wifiUI.mbox);
    style_msgbox_customize_header(wifiUI.mbox);
    lv_obj_add_event_cb(close_btn, [](lv_event_t *e) {
        WifiConnectData *data = (WifiConnectData *)lv_event_get_user_data(e);
        lv_obj_del(data->mbox);
        delete data;
    }, LV_EVENT_CLICKED, data);
}

static void update_wifi_list() {
    if (wifiUI.scanning_label) {
        lv_obj_del(wifiUI.scanning_label);
        wifiUI.scanning_label = NULL;
    }

    lv_obj_clean(wifiUI.wifi_list);

    int n = WiFi.scanComplete();
    if (n <= 0) {
        lv_list_add_text(wifiUI.wifi_list, "No networks found");
        return;
    }

    String current_ssid = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "";

    if (current_ssid.length() > 0) {
        lv_list_add_text(wifiUI.wifi_list, "Current network:");
        lv_obj_t *label = lv_list_add_btn(wifiUI.wifi_list, LV_SYMBOL_WIFI, current_ssid.c_str());
        lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
    }

    lv_list_add_text(wifiUI.wifi_list, "Available Networks:");
    bool has_visible_network = false;

    for (int i = 0; i < n; ++i) {
        String ssid = WiFi.SSID(i);
        int rssi = WiFi.RSSI(i);
        if (ssid.length() == 0 || ssid == current_ssid) continue;

        const char *strength = (rssi >= -50) ? "Excellent" :
                               (rssi >= -60) ? "Good" :
                               (rssi >= -70) ? "Fair" : "Weak";

        has_visible_network = true;
        String label = ssid + " (" + strength + ")";
        lv_obj_t *btn = lv_list_add_btn(wifiUI.wifi_list, LV_SYMBOL_WIFI, label.c_str());
        lv_obj_add_event_cb(btn, wifi_network_selected, LV_EVENT_CLICKED, NULL);
    }

    if (!has_visible_network) {
        lv_list_add_text(wifiUI.wifi_list, "No visible networks found");
    }

    WiFi.scanDelete();
}

static void check_scan_result(lv_timer_t *timer) {
    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return;
    update_wifi_list();
    lv_timer_del(timer);
    if (wifiUI.btn_back) lv_obj_clear_state(wifiUI.btn_back, LV_STATE_DISABLED);
    if (wifiUI.btn_refresh) lv_obj_clear_state(wifiUI.btn_refresh, LV_STATE_DISABLED);
}

static void back_btn_event_handler(lv_event_t *e) {
    WiFi.scanDelete();
    switch_to_screen(SCREEN_SETTING);
}

static void refresh_btn_event_handler(lv_event_t *e) {
    lv_obj_clean(wifiUI.wifi_list);
    wifiUI.scanning_label = lv_label_create(wifiUI.wifi_list);
    lv_label_set_text(wifiUI.scanning_label, "Scanning WiFi networks...");
    lv_obj_align(wifiUI.scanning_label, LV_ALIGN_CENTER, 0, 0);

    WiFi.scanDelete();
    WiFi.scanNetworks(true, true);
    lv_timer_create(check_scan_result, 500, NULL);

    lv_obj_add_state(wifiUI.btn_back, LV_STATE_DISABLED);
    lv_obj_add_state(wifiUI.btn_refresh, LV_STATE_DISABLED);
}

// Hàm tạo màn hình WiFi theo screen manager
lv_obj_t *create_wifi_screen() {

    if (wifi_screen && lv_obj_is_valid(wifi_screen)) {
        return wifi_screen;
    }

    wifi_screen = lv_obj_create(NULL);

    lv_obj_t *title = lv_label_create(wifi_screen);
    lv_label_set_text(title, "WiFi Networks");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    wifiUI.wifi_list = lv_list_create(wifi_screen);
    lv_obj_set_size(wifiUI.wifi_list, lv_pct(90), lv_pct(75));
    lv_obj_align(wifiUI.wifi_list, LV_ALIGN_CENTER, 0, 10);

    wifiUI.scanning_label = lv_label_create(wifiUI.wifi_list);
    lv_label_set_text(wifiUI.scanning_label, "Scanning WiFi networks...");
    lv_obj_align(wifiUI.scanning_label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_container = lv_obj_create(wifi_screen);
    lv_obj_set_size(btn_container, lv_pct(90), 40);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_container, 0, 0);
    lv_obj_set_style_pad_all(btn_container, 0, 0);

    wifiUI.btn_back = lv_btn_create(btn_container);
    lv_obj_set_size(wifiUI.btn_back, 60, 30);
    lv_label_set_text(lv_label_create(wifiUI.btn_back), "Back");
    lv_obj_add_event_cb(wifiUI.btn_back, back_btn_event_handler, LV_EVENT_CLICKED, NULL);

    wifiUI.btn_refresh = lv_btn_create(btn_container);
    lv_obj_set_size(wifiUI.btn_refresh, 75, 30);
    lv_label_set_text(lv_label_create(wifiUI.btn_refresh), "Refresh");
    lv_obj_add_event_cb(wifiUI.btn_refresh, refresh_btn_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_add_state(wifiUI.btn_back, LV_STATE_DISABLED);
    lv_obj_add_state(wifiUI.btn_refresh, LV_STATE_DISABLED);

    WiFi.mode(WIFI_STA);
    WiFi.scanNetworks(true, true);
    lv_timer_create(check_scan_result, 500, NULL);

    return wifi_screen;
}


/* Xử lý phân lưu thông tin WIFI */
void saveWiFiCredentials(const std::vector<WiFiCredential> &wifiList) {
    if (!SPIFFS.begin(true)) return;

    File file = SPIFFS.open("/wifi.json", FILE_WRITE);
    if (!file) return;

    StaticJsonDocument<1024> doc; // tăng dung lượng cho nhiều mạng

    JsonArray array = doc.to<JsonArray>();

    for (const auto &wifi : wifiList) {
        JsonObject obj = array.createNestedObject();
        obj["ssid"] = wifi.ssid;
        obj["password"] = wifi.password;
    }

    serializeJson(doc, file);
    file.close();
}

bool loadWiFiCredentials(std::vector<WiFiCredential> &wifiList) {
    if (!SPIFFS.begin(true)) return false;

    File file = SPIFFS.open("/wifi.json", FILE_READ);
    if (!file) return false;

    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, file);
    if (err) return false;

    JsonArray array = doc.as<JsonArray>();

    wifiList.clear();
    for (JsonObject obj : array) {
        WiFiCredential wifi;
        wifi.ssid = obj["ssid"].as<String>();
        wifi.password = obj["password"].as<String>();
        wifiList.push_back(wifi);
    }

    return true;
}

void addWiFiNetwork(const String &newSSID, const String &newPassword) {
    std::vector<WiFiCredential> wifiList;

    if (!loadWiFiCredentials(wifiList)) {
        // Nếu chưa có file, vector rỗng, bình thường
    }

    bool updated = false;
    for (auto &wifi : wifiList) {
        if (wifi.ssid == newSSID) {
            if (wifi.password != newPassword) {
                wifi.password = newPassword;  // cập nhật password
            }
            updated = true;
            break;
        }
    }

    if (!updated) {
        // SSID chưa có, thêm mới
        WiFiCredential newNetwork = { newSSID, newPassword };
        wifiList.push_back(newNetwork);
    }

    saveWiFiCredentials(wifiList);

}

// Task làm nhiêm vụ kiểm tra mạng WIFI khả dụng và tự kêt nối.
void wifi_monitor_task(void *param) {
    std::vector<WiFiCredential> wifiList;
    if (!loadWiFiCredentials(wifiList)) {
        vTaskDelete(NULL);
        return;
    }

    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi Monitor] Not connected. Scanning...");
            int n = WiFi.scanNetworks();
            bool connected = false;

            for (int i = 0; i < n && !connected; ++i) {
                String found_ssid = WiFi.SSID(i);

                for (const auto &wifi : wifiList) {
                    if (found_ssid == wifi.ssid) {
                        Serial.printf("[WiFi Monitor] Found saved network '%s'. Connecting...\n", found_ssid.c_str());
                        WiFi.begin(wifi.ssid.c_str(), wifi.password.c_str());

                        for (int j = 0; j < 10; ++j) {
                            if (WiFi.status() == WL_CONNECTED) {
                                Serial.println("[WiFi Monitor] Connected!");
                                connected = true;
                                break;
                            }
                            vTaskDelay(1000 / portTICK_PERIOD_MS);
                        }
                        if (connected) break;
                    }
                }
            }

            if (!connected) {
                Serial.println("[WiFi Monitor] No known network found or failed to connect.");
            }
        }

        // Cập nhật trạng thái WiFi trên GUI an toàn qua async
        lv_async_call([](void *) {
            update_wifi_status_ui(); // Hàm này đã kiểm tra hợp lệ bên trong
        }, nullptr);

        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

