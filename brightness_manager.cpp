#include "brightness_manager.h"
#include <driver/ledc.h>
#include <EEPROM.h>
#include "config.h"
#include "style_msgbox.h"

extern int saved_brightness;
static int _bl_pin = -1;
int percentOfSlider;

int brightness_load_from_eeprom(int default_percent) {
    int last_percent = EEPROM.read(BRIGHTNESS_ADDR);
    if (last_percent < 0 || last_percent > 100) last_percent = default_percent;
    return last_percent;
}

void brightness_manager_init(int pin_bl, int initial_percent) {
    _bl_pin = pin_bl;

    #ifdef _bl_pin
      pinMode(_bl_pin, OUTPUT);
      ledcWrite(0, map(50, 0, 100, 10, 255));
    #endif 

    // Cấu hình PWM
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t channel_conf = {
        .gpio_num = _bl_pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = map(initial_percent, 0, 100, 10, 255),
        .hpoint = 0
    };
    ledc_channel_config(&channel_conf);

    //pinMode(_bl_pin, OUTPUT);
}

// Hàm callback cho slider
static lv_obj_t * slider_label;
void brightness_slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = (lv_obj_t *)lv_event_get_target(e);
    percentOfSlider = lv_slider_get_value(slider);
    int brightness = map(percentOfSlider, 0, 100, 5, 255);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, brightness);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    
    // Hiển thị giá trị %
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", percentOfSlider);
    lv_label_set_text(slider_label, buf);
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    saved_brightness = percentOfSlider; // cập nhật lại phần trăm cho slider nếu không thoát ra quay lại là nó hiện giá trị load ban đầu
    // Ghi vào EEPROM
    EEPROM.write(BRIGHTNESS_ADDR, percentOfSlider);
    EEPROM.commit();  // Bắt buộc để ghi dữ liệu thật

}

void lv_open_brightness_screen(int default_percent)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(scr, 20, 0);

    lv_obj_t * label = lv_label_create(scr);
    lv_label_set_text(label, "Adjust Brightness");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_flex_grow(label, 0);

    lv_obj_t * slider = lv_slider_create(scr);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 60);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, default_percent, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_style_anim_duration(slider, 2000, 0);

    // Create a label below the slider to display the current slider value
    slider_label = lv_label_create(scr);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", default_percent);
    lv_label_set_text(slider_label, buf);
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // Nút quay lại màn hình Setting
    lv_obj_t * btn_back = lv_btn_create(scr);
    lv_obj_t * label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "Back");

    lv_obj_add_event_cb(btn_back, [](lv_event_t * e) {
        lv_switch_screen(SCREEN_SETTING);
    }, LV_EVENT_CLICKED, NULL);

    // Kiểm tra bộ nhớ còn lại
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
}

