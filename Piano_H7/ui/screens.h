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
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *chart_on;
    lv_obj_t *button_to_main;
    lv_obj_t *obj8;
    lv_obj_t *button_calibration_sensor_on_1;
    lv_obj_t *obj9;
    lv_obj_t *button_calibration_sensor_on_2;
    lv_obj_t *obj10;
    lv_obj_t *label_on;
    lv_obj_t *label_string_cursor;
    lv_obj_t *label_string_sensor_8;
    lv_obj_t *label_string_sensor_9;
    lv_obj_t *label_string_sensor_13;
    lv_obj_t *add_1;
    lv_obj_t *sub_1;
    lv_obj_t *add_10;
    lv_obj_t *sub_10;
    lv_obj_t *chart_off;
    lv_obj_t *button_to_main_1;
    lv_obj_t *obj11;
    lv_obj_t *sub_11;
    lv_obj_t *add_11;
    lv_obj_t *sub_2;
    lv_obj_t *add_2;
    lv_obj_t *button_calibration_sensor_on_3;
    lv_obj_t *obj12;
    lv_obj_t *button_calibration_sensor_on_4;
    lv_obj_t *obj13;
    lv_obj_t *label_on_1;
    lv_obj_t *label_string_cursor_1;
    lv_obj_t *label_string_sensor_10;
    lv_obj_t *label_string_sensor_11;
    lv_obj_t *label_string_sensor_12;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_D_MAIN = 1,
    SCREEN_ID_D_FLASH = 2,
    SCREEN_ID_D_CHART_CALIB_ON = 3,
    SCREEN_ID_D_CHART_CALIB_OFF = 4,
};

void create_screen_d_main();
void tick_screen_d_main();

void create_screen_d_flash();
void tick_screen_d_flash();

void create_screen_d_chart_calib_on();
void tick_screen_d_chart_calib_on();

void create_screen_d_chart_calib_off();
void tick_screen_d_chart_calib_off();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/