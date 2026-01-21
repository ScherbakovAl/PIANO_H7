#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_to_main_disp(lv_event_t * e);
extern void action_piano_off(lv_event_t * e);
extern void action_pre_pressure_switching(lv_event_t * e);
extern void action_to_disp_calibration_on(lv_event_t * e);
extern void action_to_disp_calibration_off(lv_event_t * e);
extern void action_to_disp_manual_edit_on(lv_event_t * e);
extern void action_to_disp_manual_edit_off(lv_event_t * e);
extern void action_to_disp_graph_resize_on(lv_event_t * e);
extern void action_to_disp_back(lv_event_t * e);
extern void action_calib_sensor_1_on(lv_event_t * e);
extern void action_calib_sensor_2_on(lv_event_t * e);
extern void action_calib_sensor_1_off(lv_event_t * e);
extern void action_calib_sensor_2_off(lv_event_t * e);
extern void action_to_disp_divisible_edit(lv_event_t * e);
extern void action_cursor_minus(lv_event_t * e);
extern void action_cursor_plus(lv_event_t * e);
extern void action_cursor_minus_7(lv_event_t * e);
extern void action_cursor_plus_7(lv_event_t * e);
extern void action_add_1(lv_event_t * e);
extern void action_add_10(lv_event_t * e);
extern void action_add_100(lv_event_t * e);
extern void action_add_1000(lv_event_t * e);
extern void action_sub_1(lv_event_t * e);
extern void action_sub_10(lv_event_t * e);
extern void action_sub_100(lv_event_t * e);
extern void action_sub_1000(lv_event_t * e);
extern void action_save_calibration(lv_event_t * e);
extern void action_s1__s2_upd(lv_event_t * e);
extern void action_top_bot(lv_event_t * e);
extern void action_div_add_100(lv_event_t * e);
extern void action_div_add_1000(lv_event_t * e);
extern void action_div_add_10000(lv_event_t * e);
extern void action_div_add_100000(lv_event_t * e);
extern void action_div_sub_100(lv_event_t * e);
extern void action_div_sub_1000(lv_event_t * e);
extern void action_div_sub_10000(lv_event_t * e);
extern void action_div_sub_100000(lv_event_t * e);
extern void action_to_disp_graph_resize_off(lv_event_t * e);
extern void action_set(lv_event_t * e);
extern void action_auto_size(lv_event_t * e);
extern void action_restore_calibration(lv_event_t * e);
extern void action_max_size_chart(lv_event_t * e);
extern void action_set_all(lv_event_t * e);
extern void action_calib_all(lv_event_t * e);
extern void action_read_all(lv_event_t * e);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/