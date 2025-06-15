#ifndef BRIGHTNESS_MANAGER_H
#define BRIGHTNESS_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>

int brightness_load_from_eeprom(int default_percent);
void brightness_manager_init(int pin_bl, int initial_percent = 50);
void brightness_slider_event_cb(lv_event_t * e);
void lv_open_brightness_screen(int default_percent);

#endif
