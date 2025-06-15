#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t *wifi_icon;
extern lv_obj_t *wifi_label;

void create_wifi_scan_screen(void);
static void update_wifi_list();

extern void wifi_monitor_task(void *param);
extern void printWiFiCredentials();

extern void update_wifi_status_ui();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // WIFI_MANAGER_H
