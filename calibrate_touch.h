#ifndef CALIBRATE_TOUCH_H
#define CALIBRATE_TOUCH_H

#include <lvgl.h>

extern void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data);
extern void lv_display_instruction();
extern void gather_cal_data();

extern void loadCalibrationFromEEPROM();

#endif