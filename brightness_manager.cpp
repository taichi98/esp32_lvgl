#include "brightness_manager.h"
#include <driver/ledc.h>
#include <EEPROM.h>
#include "config.h"
#include "style_msgbox.h"
#include "screen_manager.h"

extern int saved_brightness;

static int _bl_pin = -1;
static int percentOfSlider;
static lv_obj_t * slider_label = nullptr;
lv_obj_t * brightness_screen = nullptr;

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
}

// Callback khi kéo slider
void brightness_slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = (lv_obj_t *)lv_event_get_target(e);
    percentOfSlider = lv_slider_get_value(slider);
    int brightness = map(percentOfSlider, 0, 100, 5, 255);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, brightness);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", percentOfSlider);
    lv_label_set_text(slider_label, buf);
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    saved_brightness = percentOfSlider;
    EEPROM.write(BRIGHTNESS_ADDR, percentOfSlider);
    EEPROM.commit();
}

// Tạo và trả về màn hình chỉnh độ sáng
lv_obj_t *create_brightness_screen() {
    // Tái sử dụng màn hình nếu nó đã được khởi tạo
    if (brightness_screen && lv_obj_is_valid(brightness_screen)) {
        return brightness_screen;
    }

    brightness_screen = lv_obj_create(NULL);

    lv_obj_set_flex_flow(brightness_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(brightness_screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(brightness_screen, 20, 0);

    // Tiêu đề
    lv_obj_t * label = lv_label_create(brightness_screen);
    lv_label_set_text(label, "Adjust Brightness");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_flex_grow(label, 0);

    // Slider
    lv_obj_t * slider = lv_slider_create(brightness_screen);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 60);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, saved_brightness, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_style_anim_duration(slider, 2000, 0);

    // Label hiển thị % dưới slider
    slider_label = lv_label_create(brightness_screen);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", saved_brightness);
    lv_label_set_text(slider_label, buf);
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // Nút Back
    lv_obj_t * btn_back = lv_btn_create(brightness_screen);
    lv_obj_set_size(btn_back, 60, 30);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_t * label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "Back");

    lv_obj_add_event_cb(btn_back, [](lv_event_t * e) {
        switch_to_screen(SCREEN_SETTING);
    }, LV_EVENT_CLICKED, NULL);

    return brightness_screen;
}
