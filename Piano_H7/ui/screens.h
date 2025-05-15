#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *chart_edit_on_disp;
    lv_obj_t *chart_edit_off_disp;
    lv_obj_t *chart_manual_edit_on_disp;
    lv_obj_t *chart_manual_edit_off_disp;
    lv_obj_t *chart_graph_resize;
    lv_obj_t *divisible_edit_disp;
    lv_obj_t *button_edit_calibration_on;
    lv_obj_t *edit_calibration_label;
    lv_obj_t *button_edit_calibration_off;
    lv_obj_t *edit_calibration_label_2;
    lv_obj_t *button_edit_divisible;
    lv_obj_t *edit_divisible_label;
    lv_obj_t *button_piano_off;
    lv_obj_t *label_piano_off;
    lv_obj_t *button_edit_calibration_off_1;
    lv_obj_t *edit_calibration_label_3;
    lv_obj_t *label_divisible;
    lv_obj_t *label_string_divisible;
    lv_obj_t *label_pre_pressure;
    lv_obj_t *switcher_pre_pressure;
    lv_obj_t *deb;
    lv_obj_t *button_to_main;
    lv_obj_t *sub_10;
    lv_obj_t *add_10;
    lv_obj_t *sub_1;
    lv_obj_t *add_1;
    lv_obj_t *button_calibration_sensor_on_1;
    lv_obj_t *obj0;
    lv_obj_t *button_calibration_sensor_on_2;
    lv_obj_t *obj1;
    lv_obj_t *button_edit_cal_manual_on;
    lv_obj_t *obj2;
    lv_obj_t *button_edit_cal_manual_on_2;
    lv_obj_t *label_on;
    lv_obj_t *label_cursor;
    lv_obj_t *label_string_cursor;
    lv_obj_t *label_string_sensor_1_on;
    lv_obj_t *label_string_sensor_2_on;
    lv_obj_t *label_sensor_1_on_max;
    lv_obj_t *label_sensor_1_on_min;
    lv_obj_t *label_sensor_2_on_max;
    lv_obj_t *label_sensor_2_on_min;
    lv_obj_t *label_sensor_9;
    lv_obj_t *label_sensor_8;
    lv_obj_t *label_sensor_11;
    lv_obj_t *label_sensor_10;
    lv_obj_t *chart;
    lv_obj_t *deb_2;
    lv_obj_t *button_to_main_1;
    lv_obj_t *sub_11;
    lv_obj_t *add_11;
    lv_obj_t *sub_2;
    lv_obj_t *add_2;
    lv_obj_t *button_calibration_sensor_on_3;
    lv_obj_t *obj3;
    lv_obj_t *button_calibration_sensor_on_4;
    lv_obj_t *obj4;
    lv_obj_t *button_edit_cal_manual_on_1;
    lv_obj_t *obj5;
    lv_obj_t *button_edit_cal_manual_on_3;
    lv_obj_t *obj6;
    lv_obj_t *label_on_1;
    lv_obj_t *label_cursor_1;
    lv_obj_t *label_string_cursor_1;
    lv_obj_t *label_string_sensor_2;
    lv_obj_t *label_string_sensor_3;
    lv_obj_t *label_sensor_28;
    lv_obj_t *label_sensor_29;
    lv_obj_t *label_sensor_30;
    lv_obj_t *label_sensor_31;
    lv_obj_t *label_sensor_12;
    lv_obj_t *label_sensor_13;
    lv_obj_t *label_sensor_14;
    lv_obj_t *label_sensor_15;
    lv_obj_t *deb_1;
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
    lv_obj_t *label_on_2;
    lv_obj_t *label_cursor_2;
    lv_obj_t *label_string_cursor_2;
    lv_obj_t *label_string_sensor_4;
    lv_obj_t *label_string_sensor_5;
    lv_obj_t *label_on_5;
    lv_obj_t *deb_3;
    lv_obj_t *obj7;
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
    lv_obj_t *deb_4;
    lv_obj_t *obj8;
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
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *obj13;
    lv_obj_t *obj14;
    lv_obj_t *obj15;
    lv_obj_t *obj16;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *obj20;
    lv_obj_t *obj21;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_CHART_EDIT_ON_DISP = 2,
    SCREEN_ID_CHART_EDIT_OFF_DISP = 3,
    SCREEN_ID_CHART_MANUAL_EDIT_ON_DISP = 4,
    SCREEN_ID_CHART_MANUAL_EDIT_OFF_DISP = 5,
    SCREEN_ID_CHART_GRAPH_RESIZE = 6,
    SCREEN_ID_DIVISIBLE_EDIT_DISP = 7,
};

void create_screen_main();
void tick_screen_main();

void create_screen_chart_edit_on_disp();
void tick_screen_chart_edit_on_disp();

void create_screen_chart_edit_off_disp();
void tick_screen_chart_edit_off_disp();

void create_screen_chart_manual_edit_on_disp();
void tick_screen_chart_manual_edit_on_disp();

void create_screen_chart_manual_edit_off_disp();
void tick_screen_chart_manual_edit_off_disp();

void create_screen_chart_graph_resize();
void tick_screen_chart_graph_resize();

void create_screen_divisible_edit_disp();
void tick_screen_divisible_edit_disp();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/