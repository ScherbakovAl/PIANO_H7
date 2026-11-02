#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_to_log(lv_event_t * e);
extern void action_to_curve(lv_event_t * e);
extern void action_to_calib(lv_event_t * e);
extern void action_to_service(lv_event_t * e);
extern void action_power_off(lv_event_t * e);
extern void action_to_main(lv_event_t * e);
extern void action_curve_save(lv_event_t * e);
extern void action_curve_reset(lv_event_t * e);
extern void action_curve_tab(lv_event_t * e);
extern void action_curve_roller_changed(lv_event_t * e);
extern void action_curve_step(lv_event_t * e);
extern void action_curve_slider_changed(lv_event_t * e);
extern void action_curve_mode(lv_event_t * e);
extern void action_calib_back(lv_event_t * e);
extern void action_calib_tab(lv_event_t * e);
extern void action_calib_save(lv_event_t * e);
extern void action_calib_chart_pressed(lv_event_t * e);
extern void action_calib_step(lv_event_t * e);
extern void action_calib_follow(lv_event_t * e);
extern void action_calib_set(lv_event_t * e);
extern void action_rescan(lv_event_t * e);
extern void action_to_flash(lv_event_t * e);
extern void action_toggle_last_note(lv_event_t * e);
extern void action_restore_calibration(lv_event_t * e);
extern void action_jump_to_dfu(lv_event_t * e);
extern void action_reboot(lv_event_t * e);
extern void action_flash_back(lv_event_t * e);
extern void action_flash_step(lv_event_t * e);
extern void action_flash_boot_toggle(lv_event_t * e);
extern void action_flash_boot(lv_event_t * e);
extern void action_log_back(lv_event_t * e);
extern void action_clear_log(lv_event_t * e);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/