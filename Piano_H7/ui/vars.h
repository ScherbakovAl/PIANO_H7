#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations



// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_SENSOR_ON_1_DATA_STRING = 0,
    FLOW_GLOBAL_VARIABLE_SENSOR_ON_2_DATA_STRING = 1,
    FLOW_GLOBAL_VARIABLE_SENSOR_OFF_1_DATA_STRING = 2,
    FLOW_GLOBAL_VARIABLE_SENSOR_OFF_2_DATA_STRING = 3,
    FLOW_GLOBAL_VARIABLE_DIVISIBLE_EEZ_STRING = 4,
    FLOW_GLOBAL_VARIABLE_CURSOR_STRING = 5,
    FLOW_GLOBAL_VARIABLE_S1_ON_MIN = 6,
    FLOW_GLOBAL_VARIABLE_S1_ON_MAX = 7,
    FLOW_GLOBAL_VARIABLE_S2_ON_MIN = 8,
    FLOW_GLOBAL_VARIABLE_S2_ON_MAX = 9,
    FLOW_GLOBAL_VARIABLE_S1_OFF_MIN = 10,
    FLOW_GLOBAL_VARIABLE_S1_OFF_MAX = 11,
    FLOW_GLOBAL_VARIABLE_S2_OFF_MIN = 12,
    FLOW_GLOBAL_VARIABLE_S2_OFF_MAX = 13,
    FLOW_GLOBAL_VARIABLE_CH_O = 14,
    FLOW_GLOBAL_VARIABLE_CH_F = 15,
    FLOW_GLOBAL_VARIABLE_DISP_ON_OFF_BUTTON = 16,
    FLOW_GLOBAL_VARIABLE_TOP_BOT_STR = 17,
    FLOW_GLOBAL_VARIABLE_DEBUGG = 18,
    FLOW_GLOBAL_VARIABLE_DISP_ON_OFF_BUTTON_3 = 19,
    FLOW_GLOBAL_VARIABLE_TOP_BOT_STR_2 = 20,
    FLOW_GLOBAL_VARIABLE_TEST_TIMER2 = 21,
    FLOW_GLOBAL_VARIABLE_TEST_SPEED_FL = 22,
    FLOW_GLOBAL_VARIABLE_TEST_ENERGY_FL = 23,
    FLOW_GLOBAL_VARIABLE_TEST_MIDI_HI_FL = 24,
    FLOW_GLOBAL_VARIABLE_TEST_MIDI_LO_FL = 25,
    FLOW_GLOBAL_VARIABLE_TEST_T_OUT_FL = 26,
    FLOW_GLOBAL_VARIABLE_NOTE = 27,
    FLOW_GLOBAL_VARIABLE_CALIB_ALL_STR = 28,
    FLOW_GLOBAL_VARIABLE_MASS_STR = 29,
    FLOW_GLOBAL_VARIABLE_T1 = 30,
    FLOW_GLOBAL_VARIABLE_T2 = 31,
    FLOW_GLOBAL_VARIABLE_T3 = 32,
    FLOW_GLOBAL_VARIABLE_T4 = 33,
    FLOW_GLOBAL_VARIABLE_TIMER_DATA = 34,
    FLOW_GLOBAL_VARIABLE_CHART_CALIB_ONLINE = 35
};

// Native global variables

extern const char *get_var_sensor_on_1_data_string();
extern void set_var_sensor_on_1_data_string(const char *value);
extern const char *get_var_sensor_on_2_data_string();
extern void set_var_sensor_on_2_data_string(const char *value);
extern const char *get_var_sensor_off_1_data_string();
extern void set_var_sensor_off_1_data_string(const char *value);
extern const char *get_var_sensor_off_2_data_string();
extern void set_var_sensor_off_2_data_string(const char *value);
extern const char *get_var_divisible_eez_string();
extern void set_var_divisible_eez_string(const char *value);
extern const char *get_var_cursor_string();
extern void set_var_cursor_string(const char *value);
extern const char *get_var_s1_on_min();
extern void set_var_s1_on_min(const char *value);
extern const char *get_var_s1_on_max();
extern void set_var_s1_on_max(const char *value);
extern const char *get_var_s2_on_min();
extern void set_var_s2_on_min(const char *value);
extern const char *get_var_s2_on_max();
extern void set_var_s2_on_max(const char *value);
extern const char *get_var_s1_off_min();
extern void set_var_s1_off_min(const char *value);
extern const char *get_var_s1_off_max();
extern void set_var_s1_off_max(const char *value);
extern const char *get_var_s2_off_min();
extern void set_var_s2_off_min(const char *value);
extern const char *get_var_s2_off_max();
extern void set_var_s2_off_max(const char *value);
extern const char *get_var_ch_o();
extern void set_var_ch_o(const char *value);
extern const char *get_var_ch_f();
extern void set_var_ch_f(const char *value);
extern const char *get_var_disp_on_off_button();
extern void set_var_disp_on_off_button(const char *value);
extern const char *get_var_top_bot_str();
extern void set_var_top_bot_str(const char *value);
extern const char *get_var_debugg();
extern void set_var_debugg(const char *value);
extern const char *get_var_disp_on_off_button_3();
extern void set_var_disp_on_off_button_3(const char *value);
extern const char *get_var_top_bot_str_2();
extern void set_var_top_bot_str_2(const char *value);
extern const char *get_var_test_timer2();
extern void set_var_test_timer2(const char *value);
extern const char *get_var_test_speed_fl();
extern void set_var_test_speed_fl(const char *value);
extern const char *get_var_test_energy_fl();
extern void set_var_test_energy_fl(const char *value);
extern const char *get_var_test_midi_hi_fl();
extern void set_var_test_midi_hi_fl(const char *value);
extern const char *get_var_test_midi_lo_fl();
extern void set_var_test_midi_lo_fl(const char *value);
extern const char *get_var_test_t_out_fl();
extern void set_var_test_t_out_fl(const char *value);
extern const char *get_var_note();
extern void set_var_note(const char *value);
extern const char *get_var_calib_all_str();
extern void set_var_calib_all_str(const char *value);
extern const char *get_var_mass_str();
extern void set_var_mass_str(const char *value);
extern const char *get_var_t1();
extern void set_var_t1(const char *value);
extern const char *get_var_t2();
extern void set_var_t2(const char *value);
extern const char *get_var_t3();
extern void set_var_t3(const char *value);
extern const char *get_var_t4();
extern void set_var_t4(const char *value);
extern const char *get_var_timer_data();
extern void set_var_timer_data(const char *value);
extern const char *get_var_chart_calib_online();
extern void set_var_chart_calib_online(const char *value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/