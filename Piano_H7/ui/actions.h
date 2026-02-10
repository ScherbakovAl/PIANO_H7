#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_save_calibration(lv_event_t * e);
extern void action_restore_calibration(lv_event_t * e);
extern void action_piano_off(lv_event_t * e);
extern void action_to_disp_flash(lv_event_t * e);
extern void action_to_disp_calibration_on(lv_event_t * e);
extern void action_to_disp_calibration_off(lv_event_t * e);
extern void action_to_main_disp(lv_event_t * e);
extern void action__echo_g4s(lv_event_t * e);
extern void action_reset_bootloader_g4(lv_event_t * e);
extern void action_reset_main_to_bootloader(lv_event_t * e);
extern void action_h7_g4(lv_event_t * e);
extern void action_jump(lv_event_t * e);
extern void action_calib_sensor_on_green(lv_event_t * e);
extern void action_calib_sensor_on_red(lv_event_t * e);
extern void action_calib_sensor_off_green(lv_event_t * e);
extern void action_calib_sensor_off_red(lv_event_t * e);
extern void action_cursor_minus(lv_event_t * e);
extern void action_cursor_plus(lv_event_t * e);
extern void action_cursor_minus_7(lv_event_t * e);
extern void action_cursor_plus_7(lv_event_t * e);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/