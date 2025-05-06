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
    FLOW_GLOBAL_VARIABLE_DISP_ON_OFF = 16,
    FLOW_GLOBAL_VARIABLE_DISP_ON_OFF_BUTTON = 17,
    FLOW_GLOBAL_VARIABLE_TOP_BOT_STR = 18
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
extern const char *get_var_disp_on_off();
extern void set_var_disp_on_off(const char *value);
extern const char *get_var_disp_on_off_button();
extern void set_var_disp_on_off_button(const char *value);
extern const char *get_var_top_bot_str();
extern void set_var_top_bot_str(const char *value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/