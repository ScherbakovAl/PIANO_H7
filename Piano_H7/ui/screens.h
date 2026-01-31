#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *d_main;
    lv_obj_t *d_flash;
    lv_obj_t *d_chart_calib_on;
    lv_obj_t *d_chart_calib_off;
    lv_obj_t *d_chart_manual_edit_on;
    lv_obj_t *d_chart_graph_resize_on;
    lv_obj_t *d_chart_manual_edit_off;
    lv_obj_t *d_chart_graph_resize_off;
    lv_obj_t *divisible_edit_disp;
    lv_obj_t *button_edit_calibration_on;
    lv_obj_t *edit_calibration_label;
    lv_obj_t *button_edit_calibration_off;
    lv_obj_t *edit_calibration_label_2;
    lv_obj_t *button_piano_off;
    lv_obj_t *label_piano_off;
    lv_obj_t *button_edit_calibration_off_1;
    lv_obj_t *edit_calibration_label_3;
    lv_obj_t *button_edit_calibration_off_2;
    lv_obj_t *edit_calibration_label_4;
    lv_obj_t *deb;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *button_to_main_6;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *deb_1;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *chart_on;
    lv_obj_t *button_to_main;
    lv_obj_t *obj6;
    lv_obj_t *sub_10;
    lv_obj_t *add_10;
    lv_obj_t *sub_1;
    lv_obj_t *add_1;
    lv_obj_t *button_calibration_sensor_on_1;
    lv_obj_t *obj7;
    lv_obj_t *button_calibration_sensor_on_2;
    lv_obj_t *obj8;
    lv_obj_t *label_on;
    lv_obj_t *label_string_cursor;
    lv_obj_t *label_string_sensor_8;
    lv_obj_t *label_string_sensor_9;
    lv_obj_t *label_string_sensor_13;
    lv_obj_t *chart_off;
    lv_obj_t *button_to_main_1;
    lv_obj_t *obj9;
    lv_obj_t *sub_11;
    lv_obj_t *add_11;
    lv_obj_t *sub_2;
    lv_obj_t *add_2;
    lv_obj_t *button_calibration_sensor_on_3;
    lv_obj_t *obj10;
    lv_obj_t *button_calibration_sensor_on_4;
    lv_obj_t *obj11;
    lv_obj_t *label_on_1;
    lv_obj_t *label_string_cursor_1;
    lv_obj_t *label_string_sensor_10;
    lv_obj_t *label_string_sensor_11;
    lv_obj_t *label_string_sensor_12;
    lv_obj_t *button_to_main_2;
    lv_obj_t *sub_12;
    lv_obj_t *add_12;
    lv_obj_t *sub_3;
    lv_obj_t *add_3;
    lv_obj_t *add_14;
    lv_obj_t *add_17;
    lv_obj_t *add_18;
    lv_obj_t *add_19;
    lv_obj_t *sub_14;
    lv_obj_t *add_20;
    lv_obj_t *sub_5;
    lv_obj_t *add_5;
    lv_obj_t *s1_s2_on;
    lv_obj_t *label_string_cursor_2;
    lv_obj_t *label_string_sensor_4;
    lv_obj_t *label_string_sensor_5;
    lv_obj_t *but_send_2;
    lv_obj_t *but_send_9;
    lv_obj_t *button_edit_cal_manual_on;
    lv_obj_t *obj12;
    lv_obj_t *button_edit_cal_manual_on_2;
    lv_obj_t *button_edit_calibration_off_3;
    lv_obj_t *edit_calibration_label_5;
    lv_obj_t *button_edit_calibration_off_4;
    lv_obj_t *edit_calibration_label_6;
    lv_obj_t *test_t_out__1;
    lv_obj_t *test_speed__1;
    lv_obj_t *test_midi_hi__1;
    lv_obj_t *test_midi_lo__1;
    lv_obj_t *test_energy__1;
    lv_obj_t *t_in;
    lv_obj_t *obj13;
    lv_obj_t *obj14;
    lv_obj_t *obj15;
    lv_obj_t *obj16;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *button_to_main_4;
    lv_obj_t *sub_16;
    lv_obj_t *add_16;
    lv_obj_t *add_25;
    lv_obj_t *add_26;
    lv_obj_t *s1_s2_button_1;
    lv_obj_t *sub_7;
    lv_obj_t *add_7;
    lv_obj_t *add_27;
    lv_obj_t *add_28;
    lv_obj_t *s1_s2_button;
    lv_obj_t *label_on_4;
    lv_obj_t *label_sensor_17;
    lv_obj_t *label_sensor_21;
    lv_obj_t *label_sensor_19;
    lv_obj_t *label_sensor_23;
    lv_obj_t *label_sensor_16;
    lv_obj_t *label_sensor_20;
    lv_obj_t *label_sensor_18;
    lv_obj_t *label_sensor_22;
    lv_obj_t *obj20;
    lv_obj_t *but_send_5;
    lv_obj_t *but_send_10;
    lv_obj_t *deb_3;
    lv_obj_t *but_send;
    lv_obj_t *but_send_4;
    lv_obj_t *s1_s2_on_2;
    lv_obj_t *obj21;
    lv_obj_t *obj22;
    lv_obj_t *obj23;
    lv_obj_t *button_to_main_3;
    lv_obj_t *sub_13;
    lv_obj_t *add_13;
    lv_obj_t *sub_4;
    lv_obj_t *add_4;
    lv_obj_t *add_15;
    lv_obj_t *add_21;
    lv_obj_t *add_22;
    lv_obj_t *add_23;
    lv_obj_t *sub_15;
    lv_obj_t *add_24;
    lv_obj_t *sub_6;
    lv_obj_t *add_6;
    lv_obj_t *s1_s2_off;
    lv_obj_t *label_on_3;
    lv_obj_t *label_cursor_3;
    lv_obj_t *label_string_cursor_3;
    lv_obj_t *label_string_sensor_6;
    lv_obj_t *label_string_sensor_7;
    lv_obj_t *label_on_6;
    lv_obj_t *but_send_3;
    lv_obj_t *but_send_8;
    lv_obj_t *deb_4;
    lv_obj_t *button_edit_cal_manual_on_1;
    lv_obj_t *obj24;
    lv_obj_t *button_edit_cal_manual_on_3;
    lv_obj_t *obj25;
    lv_obj_t *obj26;
    lv_obj_t *button_to_main_5;
    lv_obj_t *sub_17;
    lv_obj_t *add_29;
    lv_obj_t *add_30;
    lv_obj_t *add_31;
    lv_obj_t *s1_s2_button_2;
    lv_obj_t *sub_8;
    lv_obj_t *add_8;
    lv_obj_t *add_32;
    lv_obj_t *add_33;
    lv_obj_t *s1_s2_button_3;
    lv_obj_t *label_on_7;
    lv_obj_t *label_sensor_24;
    lv_obj_t *label_sensor_25;
    lv_obj_t *label_sensor_26;
    lv_obj_t *label_sensor_27;
    lv_obj_t *label_sensor_32;
    lv_obj_t *label_sensor_33;
    lv_obj_t *label_sensor_34;
    lv_obj_t *label_sensor_35;
    lv_obj_t *obj27;
    lv_obj_t *but_send_7;
    lv_obj_t *but_send_11;
    lv_obj_t *deb_5;
    lv_obj_t *but_send_1;
    lv_obj_t *s1_s2_on_1;
    lv_obj_t *but_send_6;
    lv_obj_t *obj28;
    lv_obj_t *obj29;
    lv_obj_t *obj30;
    lv_obj_t *obj31;
    lv_obj_t *obj32;
    lv_obj_t *obj33;
    lv_obj_t *obj34;
    lv_obj_t *obj35;
    lv_obj_t *obj36;
    lv_obj_t *obj37;
    lv_obj_t *obj38;
    lv_obj_t *obj39;
    lv_obj_t *obj40;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_D_MAIN = 1,
    SCREEN_ID_D_FLASH = 2,
    SCREEN_ID_D_CHART_CALIB_ON = 3,
    SCREEN_ID_D_CHART_CALIB_OFF = 4,
    SCREEN_ID_D_CHART_MANUAL_EDIT_ON = 5,
    SCREEN_ID_D_CHART_GRAPH_RESIZE_ON = 6,
    SCREEN_ID_D_CHART_MANUAL_EDIT_OFF = 7,
    SCREEN_ID_D_CHART_GRAPH_RESIZE_OFF = 8,
    SCREEN_ID_DIVISIBLE_EDIT_DISP = 9,
};

void create_screen_d_main();
void tick_screen_d_main();

void create_screen_d_flash();
void tick_screen_d_flash();

void create_screen_d_chart_calib_on();
void tick_screen_d_chart_calib_on();

void create_screen_d_chart_calib_off();
void tick_screen_d_chart_calib_off();

void create_screen_d_chart_manual_edit_on();
void tick_screen_d_chart_manual_edit_on();

void create_screen_d_chart_graph_resize_on();
void tick_screen_d_chart_graph_resize_on();

void create_screen_d_chart_manual_edit_off();
void tick_screen_d_chart_manual_edit_off();

void create_screen_d_chart_graph_resize_off();
void tick_screen_d_chart_graph_resize_off();

void create_screen_divisible_edit_disp();
void tick_screen_divisible_edit_disp();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/