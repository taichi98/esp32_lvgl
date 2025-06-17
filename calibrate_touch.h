#ifndef CALIBRATE_TOUCH_H
#define CALIBRATE_TOUCH_H

#include <lvgl.h>

extern void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data);
lv_obj_t *create_calibration_screen();
extern void gather_cal_data();

extern void loadCalibrationFromEEPROM();

#endif