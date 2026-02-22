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
    FLOW_GLOBAL_VARIABLE_DEBUGG = 0,
    FLOW_GLOBAL_VARIABLE_CURSOR_STRING = 1,
    FLOW_GLOBAL_VARIABLE_CHART_CALIB_ONLINE = 2,
    FLOW_GLOBAL_VARIABLE_L = 3,
    FLOW_GLOBAL_VARIABLE_R = 4,
    FLOW_GLOBAL_VARIABLE_SENSOR_ON_1_DATA_STRING = 5,
    FLOW_GLOBAL_VARIABLE_SENSOR_ON_2_DATA_STRING = 6,
    FLOW_GLOBAL_VARIABLE_SENSOR_OFF_1_DATA_STRING = 7,
    FLOW_GLOBAL_VARIABLE_SENSOR_OFF_2_DATA_STRING = 8,
    FLOW_GLOBAL_VARIABLE_N_CHIP = 9,
    FLOW_GLOBAL_VARIABLE_N_COMP = 10
};

// Native global variables

extern const char *get_var_debugg();
extern void set_var_debugg(const char *value);
extern const char *get_var_cursor_string();
extern void set_var_cursor_string(const char *value);
extern const char *get_var_chart_calib_online();
extern void set_var_chart_calib_online(const char *value);
extern const char *get_var_l();
extern void set_var_l(const char *value);
extern const char *get_var_r();
extern void set_var_r(const char *value);
extern const char *get_var_sensor_on_1_data_string();
extern void set_var_sensor_on_1_data_string(const char *value);
extern const char *get_var_sensor_on_2_data_string();
extern void set_var_sensor_on_2_data_string(const char *value);
extern const char *get_var_sensor_off_1_data_string();
extern void set_var_sensor_off_1_data_string(const char *value);
extern const char *get_var_sensor_off_2_data_string();
extern void set_var_sensor_off_2_data_string(const char *value);
extern const char *get_var_n_chip();
extern void set_var_n_chip(const char *value);
extern const char *get_var_n_comp();
extern void set_var_n_comp(const char *value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/