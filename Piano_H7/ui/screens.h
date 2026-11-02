#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *d_main;
    lv_obj_t *d_curve;
    lv_obj_t *d_calib;
    lv_obj_t *d_service;
    lv_obj_t *d_flash;
    lv_obj_t *d_log;
    lv_obj_t *main_status_chips;
    lv_obj_t *main_status_usb;
    lv_obj_t *main_status_calib;
    lv_obj_t *main_status_warn;
    lv_obj_t *main_status_warn_label;
    lv_obj_t *main_curve_tile;
    lv_obj_t *main_curve_name;
    lv_obj_t *main_curve_chart;
    lv_obj_t *main_curve_modes;
    lv_obj_t *main_last_note;
    lv_obj_t *main_last_note_label;
    lv_obj_t *main_last_note_bar;
    lv_obj_t *curve_save;
    lv_obj_t *curve_save_label;
    lv_obj_t *curve_reset;
    lv_obj_t *curve_tab_on;
    lv_obj_t *curve_tab_off;
    lv_obj_t *curve_tab_lift;
    lv_obj_t *curve_chart;
    lv_obj_t *curve_axis_min;
    lv_obj_t *curve_info;
    lv_obj_t *curve_axis_max;
    lv_obj_t *curve_roller;
    lv_obj_t *curve_mode_info;
    lv_obj_t *curve_sliders;
    lv_obj_t *curve_row_fast;
    lv_obj_t *curve_v_fast;
    lv_obj_t *curve_s_fast;
    lv_obj_t *curve_row_slow;
    lv_obj_t *curve_v_slow;
    lv_obj_t *curve_s_slow;
    lv_obj_t *curve_row_x0;
    lv_obj_t *curve_v_x0;
    lv_obj_t *curve_s_x0;
    lv_obj_t *curve_fixed;
    lv_obj_t *curve_mode_panel;
    lv_obj_t *curve_mode_lin;
    lv_obj_t *curve_mode_log;
    lv_obj_t *calib_tab_on;
    lv_obj_t *calib_tab_off;
    lv_obj_t *calib_save;
    lv_obj_t *calib_save_label;
    lv_obj_t *calib_chart;
    lv_obj_t *calib_key;
    lv_obj_t *calib_key_info;
    lv_obj_t *calib_live;
    lv_obj_t *calib_neighbors;
    lv_obj_t *calib_margin;
    lv_obj_t *calib_follow;
    lv_obj_t *calib_scale;
    lv_obj_t *calib_set_g;
    lv_obj_t *calib_g_change;
    lv_obj_t *calib_set_r;
    lv_obj_t *calib_r_change;
    lv_obj_t *service_chips_summary;
    lv_obj_t *service_chip_grid;
    lv_obj_t *service_warn;
    lv_obj_t *service_last_note_state;
    lv_obj_t *flash_state;
    lv_obj_t *flash_step1;
    lv_obj_t *flash_step2;
    lv_obj_t *flash_step3;
    lv_obj_t *flash_boot_toggle_label;
    lv_obj_t *flash_log;
    lv_obj_t *flash_boot_panel;
    lv_obj_t *log_counters;
    lv_obj_t *log_text;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_D_MAIN = 1,
    SCREEN_ID_D_CURVE = 2,
    SCREEN_ID_D_CALIB = 3,
    SCREEN_ID_D_SERVICE = 4,
    SCREEN_ID_D_FLASH = 5,
    SCREEN_ID_D_LOG = 6,
};

void create_screen_d_main();
void tick_screen_d_main();

void create_screen_d_curve();
void tick_screen_d_curve();

void create_screen_d_calib();
void tick_screen_d_calib();

void create_screen_d_service();
void tick_screen_d_service();

void create_screen_d_flash();
void tick_screen_d_flash();

void create_screen_d_log();
void tick_screen_d_log();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/