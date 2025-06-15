#include <WiFi.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <EEPROM.h>
#include "style_msgbox.h"
#include "wifi_manager.h"
#include "brightness_manager.h"
#include "calibrate_touch.h"
#include "config.h"
#include "resources.h"

// Biến toàn cục xử lý phần delay thời gian trong chức năng cân chỉnh màn hình.
static uint32_t instruction_start_time = 0;
static bool calibration_in_progress = false;

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// Touchscreen coordinates: (x, y) and pressure (z)
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

int saved_brightness;

// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

void lv_setting_box(void) {
    lv_obj_t * setting = lv_msgbox_create(lv_screen_active());
    lv_obj_set_style_clip_corner(setting, true, 0);

    /* setting fixed size */
    lv_obj_set_size(setting, 300, 220);

    /* setting's titlebar/header */
    lv_msgbox_add_title(setting, "Setting");
    lv_msgbox_add_close_button(setting);
    style_msgbox_customize_header(setting); 
    // Sự kiện khi đóng hộp thoại Setting (nút close)
    lv_obj_add_event_cb(setting, [](lv_event_t * e) {
        if (lv_event_get_code(e) == LV_EVENT_DELETE) {
            //lv_obj_clean(lv_scr_act());
            lv_create_main_gui();  // Quay lại giao diện chính
        }
    }, LV_EVENT_DELETE, NULL);

    /* setting's content*/
    lv_obj_t * content = lv_msgbox_get_content(setting);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_right(content, -1, LV_PART_SCROLLBAR);

    // Mục WiFi
    create_setting_item(content, LV_SYMBOL_WIFI, "WiFi Settings", [](lv_event_t * e) {
        create_wifi_scan_screen();
        lv_obj_del((lv_obj_t *)lv_event_get_target(e));
    }, NULL);

    // Mục Brightness
    create_setting_item(content, LV_FONT_BRIGHTNESS, "Brightness", [](lv_event_t * e) {
        lv_open_brightness_screen(saved_brightness);
        lv_obj_del((lv_obj_t *)lv_event_get_target(e));
    }, &lv_brightness_16);  

    //debug ktra bộ nhớ Heap
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
}

// Gọi hàm nếu button Setting được nhấn
static void event_handler_settingbtn(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_CLICKED) {
    wifi_icon = NULL;
    wifi_label = NULL;
    lv_obj_clean(lv_scr_act());
  
    lv_setting_box();
  }
}

// Gọi hàm nếu button Calibrate được nhấn.
static void event_handler_calibratebtn(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_CLICKED) {
    lv_obj_clean(lv_scr_act());
    lv_display_instruction();
    
    instruction_start_time = millis();
    calibration_in_progress = true;  // Bật cờ bắt đầu quá trình calibrate
  }
}

// Hàm kiểm tra timeout (chỉ chạy khi đang calibrate)
void check_calibration_screen_timeout() {
  if (calibration_in_progress && (millis() - instruction_start_time >= 5000)) {
    calibration_in_progress = false;  // Tắt cờ sau khi chuyển màn hình
    gather_cal_data();
  }
}

void lv_create_main_gui(void) {

  lv_async_call([](void *) {
      update_wifi_status_ui();
  }, nullptr);

  lv_obj_t * text_label = lv_label_create(lv_screen_active());
  lv_label_set_long_mode(text_label, LV_LABEL_LONG_WRAP);    // Breaks the long lines
  lv_label_set_text(text_label, "MAIN MENU");
  lv_obj_set_width(text_label, 150);    // Set smaller width to make the lines wrap
  lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(text_label, LV_ALIGN_CENTER, 0, -100);

  wifi_icon = lv_img_create(lv_scr_act());
  lv_img_set_src(wifi_icon, &no_wifi); // Mặc định chưa kết nối
  lv_obj_align(wifi_icon, LV_ALIGN_TOP_RIGHT, -70, 10);  // <-- Lùi vào 70px thay vì 60

  wifi_label = lv_label_create(lv_scr_act());
  lv_label_set_text(wifi_label, "No wifi");
  lv_obj_set_width(wifi_label, 60);
  lv_label_set_long_mode(wifi_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_align_to(wifi_label, wifi_icon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);  // Gắn bên phải icon
  
  // Tạo nút Setting
  create_icon_button(&settings_icon, "Setting", LV_ALIGN_LEFT_MID, 20, -35, event_handler_settingbtn);

  // Button clock
  create_icon_button(&clock_icon, "Clock", LV_ALIGN_LEFT_MID, 120, -35, NULL);

  // Weather icon
  create_icon_button(&weather_icon, "Weather", LV_ALIGN_LEFT_MID, 220, -35, NULL);

  // Tạo nút Calibrate
  create_icon_button(&touch_cali, "Calibrate", LV_ALIGN_LEFT_MID, 20, 60, event_handler_calibratebtn); 

  // SD Card icon
  create_icon_button(&micro_sd_card, "SD Card", LV_ALIGN_LEFT_MID, 120, 60, NULL);

  // Kiểm tra bộ nhớ còn lại
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());

}

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);

  // Start LVGL
  lv_init();
  lv_log_register_print_cb(log_print);

  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(2);

  // Khởi tạo đối tượng
  lv_display_t * disp;
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
    
  // Initialize an LVGL input device object (Touchscreen)
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read); // Gọi hàm đọc giá trị Cảm ứng
  lv_create_main_gui(); // Gọi hàm vẽ giao diện UI
  
  EEPROM.begin(EEPROM_SIZE);  // Khởi tạo EEPROM
  saved_brightness = brightness_load_from_eeprom(50);  // Nếu chưa có, dùng 50%
  brightness_manager_init(TFT_BL, saved_brightness);
  loadCalibrationFromEEPROM();
  init_styles();

  // Thiêt lập phần xử lý tự kết nối WIFI độc lập trên Core 0
  xTaskCreatePinnedToCore(
    wifi_monitor_task, 
    "WiFiMonitor", 
    4096, 
    NULL, 
    1, 
    NULL, 
    0
  ); // Core 0

}

void loop() {

  // Chỉ kiểm tra timeout nếu đang trong quá trình calibrate
  if (calibration_in_progress) {
    check_calibration_screen_timeout();
  }

  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);
  delay(5); // Nhẹ CPU  
}