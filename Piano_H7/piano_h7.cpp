/*
 * midi_keyboard_h7-2.cpp
 *
 *  Created on: Apr 8, 2025
 *      Author: sche
 */

#include "piano_h7.hpp"


#define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))
#define BUFF_SIZE (480 * 10 * BYTES_PER_PIXEL)
static lv_color16_t buf_1[BUFF_SIZE];
static lv_color16_t buf_2[BUFF_SIZE];

lv_display_t* disp;
lv_indev_t* indev;
lv_obj_t* btn;
lv_obj_t* label_btn;

void h7() {
	// LCD init
	LL_SPI_Enable(SPI3);
	LL_SPI_StartMasterTransfer(SPI3);
	LCD_Init();

	// TOUCH init
	// LL_I2C_Enable(I2C5);
	FT6336_Init();

	// LVGL init
	LL_TIM_EnableCounter(TIM4); // для LVGL
	LL_TIM_EnableIT_UPDATE(TIM4);
	lv_init();

	// DISP start
	disp = lv_display_create(480, 320);
	lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
	lv_display_set_flush_cb(disp, my_flush_cb);
	lv_display_set_buffers(disp, buf_1, buf_2, sizeof(buf_1),
		LV_DISPLAY_RENDER_MODE_PARTIAL);


	// TOUCH start
	LL_TIM_EnableIT_UPDATE(TIM6); // для TOUCH
	indev = lv_indev_create();
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, my_input_read);
	// -----

	// GUI start
	ui_init();

	// USB init
	LL_TIM_EnableCounter(TIM2); // счётчик
	tud_init(BOARD_TUD_RHPORT);
	// tusb_init();

	LL_mDelay(300);
	send_test_midi();

	while (1) {
		tud_task();
		lv_timer_handler();
		ui_tick();
		send_test_midi();
	}
}

// typedef void (*lv_display_flush_cb_t)(lv_display_t * disp, const lv_area_t * area, uint16_t * px_map); >>>  lv_display.h ( uint16_t !!! ) !!
void my_flush_cb(lv_display_t* disp, const lv_area_t* area, uint16_t* color_p) {
	LCD_SetWindows(area->x1, area->y1, area->x2, area->y2);
	int height = area->y2 - area->y1 + 1;
	int width = area->x2 - area->x1 + 1;
	for (int i = 0; i < width * height; i++) {
		LCD_Send_Data_16(color_p);
		++color_p;
	}
	// Send_DMA_Data8(color_p, width  * height);

	lv_display_flush_ready(disp);
}

void send_test_midi() {
	if (TIM2->CNT > 3000000) {
		TIM2->CNT = 0;
		uint8_t const cable_num = 0;
		uint8_t note_buf[] = { 0xB0, 0x58, 125, 0x90, 0x3E, 0x36, 0xB0, 0x58, 125, 0x80, 0x3E, 0x36 };
		const int bufsize = sizeof(note_buf);
		tud_midi_stream_write(cable_num, note_buf, bufsize);

	}
}
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
#include "vars.h"
#include <string>

std::string ch_o;
extern "C" const char* get_var_ch_o() {
	return ch_o.c_str();
}
extern "C" void set_var_ch_o(const char* value) {
	ch_o = value;
}

std::string ch_f;
extern "C" const char* get_var_ch_f() {
	return ch_f.c_str();
}
extern "C" void set_var_ch_f(const char* value) {
	ch_f = value;
}

std::string s1_on_min;
extern "C" const char* get_var_s1_on_min() {
	return s1_on_min.c_str();
}
extern "C" void set_var_s1_on_min(const char* value) {
	s1_on_min = value;
}

std::string s1_on_max;
extern "C" const char* get_var_s1_on_max() {
	return s1_on_max.c_str();
}
extern "C" void set_var_s1_on_max(const char* value) {
	s1_on_max = value;
}

std::string s2_on_min;
extern "C" const char* get_var_s2_on_min() {
	return s2_on_min.c_str();
}
extern "C" void set_var_s2_on_min(const char* value) {
	s2_on_min = value;
}

std::string s2_on_max;
extern "C" const char* get_var_s2_on_max() {
	return s2_on_max.c_str();
}
extern "C" void set_var_s2_on_max(const char* value) {
	s2_on_max = value;
}

std::string s1_off_min;
extern "C" const char* get_var_s1_off_min() {
	return s1_off_min.c_str();
}
extern "C" void set_var_s1_off_min(const char* value) {
	s1_off_min = value;
}

std::string s1_off_max;
extern "C" const char* get_var_s1_off_max() {
	return s1_off_max.c_str();
}
extern "C" void set_var_s1_off_max(const char* value) {
	s1_off_max = value;
}

std::string s2_off_min;
extern "C" const char* get_var_s2_off_min() {
	return s2_off_min.c_str();
}
extern "C" void set_var_s2_off_min(const char* value) {
	s2_off_min = value;
}

std::string s2_off_max;
extern "C" const char* get_var_s2_off_max() {
	return s2_off_max.c_str();
}
extern "C" void set_var_s2_off_max(const char* value) {
	s2_off_max = value;
}

std::string sensor_on_1_data_string;
extern "C" const char* get_var_sensor_on_1_data_string() {
	sensor_on_1_data_string.clear();
	sensor_on_1_data_string = std::to_string(compsCHART_ON_1[cursor]);
	return sensor_on_1_data_string.c_str();
}
extern "C" void set_var_sensor_on_1_data_string(const char* value) {
	sensor_on_1_data_string = value;
}

std::string sensor_on_2_data_string;
extern "C" const char* get_var_sensor_on_2_data_string() {
	sensor_on_2_data_string.clear();
	sensor_on_2_data_string = std::to_string(compsCHART_ON_2[cursor]);
	return sensor_on_2_data_string.c_str();
}
extern "C" void set_var_sensor_on_2_data_string(const char* value) {
	sensor_on_2_data_string = value;
}

std::string sensor_off_1_data_string;
extern "C" const char* get_var_sensor_off_1_data_string() {
	sensor_off_1_data_string.clear();
	sensor_off_1_data_string = std::to_string(compsCHART_OFF_1[cursor]);
	return sensor_off_1_data_string.c_str();
}
extern "C" void set_var_sensor_off_1_data_string(const char* value) {
	sensor_off_1_data_string = value;
}

std::string sensor_off_2_data_string;
extern "C" const char* get_var_sensor_off_2_data_string() {
	sensor_off_2_data_string.clear();
	sensor_off_2_data_string = std::to_string(compsCHART_OFF_2[cursor]);
	return sensor_off_2_data_string.c_str();
}
extern "C" void set_var_sensor_off_2_data_string(const char* value) {
	sensor_off_2_data_string = value;
}

std::string divisible_eez_string;
extern "C" const char* get_var_divisible_eez_string() {
	divisible_eez_string.clear();
	divisible_eez_string = std::to_string(divis);
	return divisible_eez_string.c_str();
}
extern "C" void set_var_divisible_eez_string(const char* value) {
	divisible_eez_string = value;
}

std::string cursor_string;
extern "C" const char* get_var_cursor_string() {
	cursor_string.clear();
	cursor_string = std::to_string(cursor);
	return cursor_string.c_str();
}
extern "C" void set_var_cursor_string(const char* value) {
	cursor_string = value;
}

std::string disp_on_off;
extern "C" const char* get_var_disp_on_off() {
	return disp_on_off.c_str();
}
extern "C" void set_var_disp_on_off(const char* value) {
	disp_on_off = value;
}

std::string disp_on_off_button;
extern "C" const char* get_var_disp_on_off_button() {
	return disp_on_off_button.c_str();
}
extern "C" void set_var_disp_on_off_button(const char* value) {
	disp_on_off_button = value;
}

std::string top_bot_str;
extern "C" const char* get_var_top_bot_str() {
	return top_bot_str.c_str();
}
extern "C" void set_var_top_bot_str(const char* value) {
	top_bot_str = value;
}

// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
#include "actions.h"

extern "C" void action_to_main_disp(lv_event_t* e) {
	loadScreen(SCREEN_ID_MAIN);
}

extern "C" void action_piano_off(lv_event_t* e) {
	// TODO: Implement action piano_off here
}

extern "C" void action_pre_pressure_switching(lv_event_t* e) {
	// TODO: Implement action pre_pressure_switching here
}

lv_chart_series_t* ser_1;
lv_chart_series_t* ser_2;
lv_chart_cursor_t* c1;
lv_point_t* pp;
int fl = 1;

extern "C" void action_to_disp_calibration_on(lv_event_t* e) {
	fl_disp = 0;
	manual_edit_on();
}

void manual_edit_on() {
	comp_to_chart();
	start_chart();
	lv_chart_set_series_ext_y_array(objects.chart, ser_1, compsCHART_ON_1);
	lv_chart_set_series_ext_y_array(objects.chart, ser_2, compsCHART_ON_2);
	lv_obj_set_parent(objects.chart, objects.chart_edit_on_disp);
	loadScreen(SCREEN_ID_CHART_EDIT_ON_DISP);
}

extern "C" void action_to_disp_calibration_off(lv_event_t* e) {
	fl_disp = 1;
	manual_edit_off();
}

void manual_edit_off() {
	comp_to_chart();
	start_chart();
	lv_chart_set_series_ext_y_array(objects.chart, ser_1, compsCHART_OFF_1);
	lv_chart_set_series_ext_y_array(objects.chart, ser_2, compsCHART_OFF_2);
	lv_obj_set_parent(objects.chart, objects.chart_edit_off_disp);
	loadScreen(SCREEN_ID_CHART_EDIT_OFF_DISP);
}

extern "C" void action_to_disp_manual_edit_on(lv_event_t* e) {
	fl_disp = 0;
	comp_to_chart();
	lv_chart_set_series_ext_y_array(objects.chart, ser_1, compsCHART_ON_1);
	lv_chart_set_series_ext_y_array(objects.chart, ser_2, compsCHART_ON_2);
	lv_obj_set_parent(objects.chart, objects.chart_manual_edit_on_disp);
	loadScreen(SCREEN_ID_CHART_MANUAL_EDIT_ON_DISP);
}

extern "C" void action_to_disp_manual_edit_off(lv_event_t* e) {
	fl_disp = 1;
	comp_to_chart();
	lv_chart_set_series_ext_y_array(objects.chart, ser_1, compsCHART_OFF_1);
	lv_chart_set_series_ext_y_array(objects.chart, ser_2, compsCHART_OFF_2);
	lv_obj_set_parent(objects.chart, objects.chart_manual_edit_off_disp);
	loadScreen(SCREEN_ID_CHART_MANUAL_EDIT_OFF_DISP);
}

extern "C" void action_to_disp_graph_resize(lv_event_t* e) {
	disp_on_off.clear();

	comp_to_chart();
	if (fl_disp) {
		disp_on_off = "OFF";
		s1_on_min.clear();
		s1_on_min = std::to_string(off_green_min);
		s1_on_max.clear();
		s1_on_max = std::to_string(off_green_max);
		s2_on_min.clear();
		s2_on_min = std::to_string(off_red_min);
		s2_on_max.clear();
		s2_on_max = std::to_string(off_red_max);
		lv_chart_set_series_ext_y_array(objects.chart, ser_1, compsCHART_OFF_1);
		lv_chart_set_series_ext_y_array(objects.chart, ser_2, compsCHART_OFF_2);
	}
	else {
		disp_on_off = "ON";
		s1_on_min.clear();
		s1_on_min = std::to_string(on_green_min);
		s1_on_max.clear();
		s1_on_max = std::to_string(on_green_max);
		s2_on_min.clear();
		s2_on_min = std::to_string(on_red_min);
		s2_on_max.clear();
		s2_on_max = std::to_string(on_red_max);
		lv_chart_set_series_ext_y_array(objects.chart, ser_1, compsCHART_ON_1);
		lv_chart_set_series_ext_y_array(objects.chart, ser_2, compsCHART_ON_2);
	}
	lv_obj_set_parent(objects.chart, objects.chart_graph_resize);
	loadScreen(SCREEN_ID_CHART_GRAPH_RESIZE);
}

extern "C" void action_to_disp_back(lv_event_t* e) {
	fl_disp ? manual_edit_off() : manual_edit_on();
}

extern "C" void action_calib_sensor_1_on(lv_event_t* e) {
	// TODO: Implement action calib_sensor_1_on here
}

extern "C" void action_calib_sensor_2_on(lv_event_t* e) {
	// TODO: Implement action calib_sensor_2_on here
}

extern "C" void action_calib_sensor_1_off(lv_event_t* e) {
	// TODO: Implement action calib_sensor_1_off here
}

extern "C" void action_calib_sensor_2_off(lv_event_t* e) {
	// TODO: Implement action calib_sensor_2_off here
}

extern "C" void action_to_disp_divisible_edit(lv_event_t* e) {
	loadScreen(SCREEN_ID_DIVISIBLE_EDIT_DISP);
}

extern "C" void action_cursor_minus(lv_event_t* e) {
	if (cursor > 0) {
		--cursor;
	}
	lv_chart_set_cursor_point(objects.chart, c1, ser_1, cursor);
}

extern "C" void action_cursor_plus(lv_event_t* e) {
	if (cursor < 90) {
		++cursor;
	}
	lv_chart_set_cursor_point(objects.chart, c1, ser_1, cursor);
}

extern "C" void action_cursor_minus10(lv_event_t* e) {
	if (cursor > 9) {
		cursor -= 10;
	}
	lv_chart_set_cursor_point(objects.chart, c1, ser_1, cursor);
}

extern "C" void action_cursor_plus10(lv_event_t* e) {
	if (cursor < 81) {
		cursor += 10;
	}
	lv_chart_set_cursor_point(objects.chart, c1, ser_1, cursor);
}

extern "C" void action_add_1(lv_event_t* e) {
	char_correction(1, plus);
}

extern "C" void action_add_10(lv_event_t* e) {
	char_correction(10, plus);
}

extern "C" void action_add_100(lv_event_t* e) {
	char_correction(100, plus);
}

extern "C" void action_add_1000(lv_event_t* e) {
	char_correction(1000, plus);
}

extern "C" void action_sub_1(lv_event_t* e) {
	char_correction(1, minus);
}

extern "C" void action_sub_10(lv_event_t* e) {
	char_correction(10, minus);
}

extern "C" void action_sub_100(lv_event_t* e) {
	char_correction(100, minus);
}

extern "C" void action_sub_1000(lv_event_t* e) {
	char_correction(1000, minus);
}

extern "C" void action_save_calibration(lv_event_t* e) {
	// TODO: Implement action save_calibration here
}

extern "C" void action_s1__s2_upd(lv_event_t* e) {
	ch_o.clear();
	if (lv_obj_get_state(objects.s1_s2_on) == 16) { // == зелёная
		ch_o = "green";
		fl_on = 1;
	}
	else if (lv_obj_get_state(objects.s1_s2_on) == 17) { // == красная
		ch_o = "red";
		fl_on = 2;
	}
	else {
		ch_o = std::to_string(lv_obj_get_state(objects.s1_s2_on));
		fl_on = 0;
	}
	ch_f.clear();
	if (lv_obj_get_state(objects.s1_s2_off) == 16) { // == зелёная
		ch_f = "green";
		fl_off = 1;
	}
	else if (lv_obj_get_state(objects.s1_s2_off) == 17) { // == красная
		ch_f = "red";
		fl_off = 2;
	}
	else {
		ch_f = std::to_string(lv_obj_get_state(objects.s1_s2_off));
		fl_off = 0;
	}
	disp_on_off_button.clear();
	if (lv_obj_get_state(objects.s1_s2_button) == 16) { // == зелёная
		disp_on_off_button = "green";
		s1_s2 = 0;
	}
	else if (lv_obj_get_state(objects.s1_s2_button) == 17) { // == красная
		disp_on_off_button = "red";
		s1_s2 = 1;
	}
	else {
		disp_on_off_button = std::to_string(
			lv_obj_get_state(objects.s1_s2_button));
	}
}

extern "C" void action_top_bot(lv_event_t* e) {
	top_bot_str.clear();
	if (lv_obj_get_state(objects.s1_s2_button_1) == 16) { // == зелёная
		top_bot_str = "top";
		top_bot = 0;
	}
	else if (lv_obj_get_state(objects.s1_s2_button_1) == 17) { // == красная
		top_bot_str = "bottom";
		top_bot = 1;
	}
	else {
		top_bot_str = std::to_string(lv_obj_get_state(objects.s1_s2_button_1));
	}
}

extern "C" void action_div_add_100(lv_event_t* e) {
	divis += 100000;
}

extern "C" void action_div_add_1000(lv_event_t* e) {
	divis += 1000000;
}

extern "C" void action_div_add_10000(lv_event_t* e) {
	divis += 10000000;
}

extern "C" void action_div_add_100000(lv_event_t* e) {
	divis += 100000000;
}

extern "C" void action_div_sub_100(lv_event_t* e) {
	divis -= 100000;
}

extern "C" void action_div_sub_1000(lv_event_t* e) {
	divis -= 1000000;
}

extern "C" void action_div_sub_10000(lv_event_t* e) {
	divis -= 10000000;
}

extern "C" void action_div_sub_100000(lv_event_t* e) {
	divis -= 100000000;
}

// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

void my_input_read(lv_indev_t* indev, lv_indev_data_t* data) {
	if (touchpad_pressed) {
		TouchPoints_HandleTypeDef TP = FT6336_GetTouchPoint();
		data->point.x = TP.point1_x;
		data->point.y = TP.point1_y;
		data->state = LV_INDEV_STATE_PRESSED;
	}
	else {
		data->state = LV_INDEV_STATE_RELEASED;
	}
}

void comp_to_chart() {
	int c = 0; // 0 <> allChipCount
	int k = 0; // 0 <> 7
	for (int i = 0; i < allKeys; ++i) {
		compsCHART_ON_1[i] = comparator[c].comp[k][0];
		compsCHART_ON_2[i] = comparator[c].comp[k][1];
		++k;
		if (k > 6) {
			k = 0;
			++c;
		}
	}
	k = 0;
	for (int i = 0; i < allKeys; ++i) {
		compsCHART_OFF_1[i] = comparator[c].comp[k][0];
		compsCHART_OFF_2[i] = comparator[c].comp[k][1];
		++k;
		if (k > 6) {
			k = 0;
			++c;
		}
	}
	if (fl_disp) {
		lv_chart_set_axis_range(objects.chart, LV_CHART_AXIS_PRIMARY_Y,
			off_green_min, off_green_max);
		lv_chart_set_axis_range(objects.chart, LV_CHART_AXIS_SECONDARY_Y,
			off_red_min, off_red_max);
	}
	else {
		lv_chart_set_axis_range(objects.chart, LV_CHART_AXIS_PRIMARY_Y,
			on_green_min, on_green_max);
		lv_chart_set_axis_range(objects.chart, LV_CHART_AXIS_SECONDARY_Y,
			on_red_min, on_red_max);
	}
	check_max_min();
}

void chart_to_comp() {
	int c = 0; // 0 <> allChipCount
	int k = 0; // 0 <> 7
	for (int i = 0; i < allKeys; ++i) {
		comparator[c].comp[k][0] = compsCHART_ON_1[i];
		comparator[c].comp[k][1] = compsCHART_ON_2[i];
		++k;
		if (k > 6) {
			k = 0;
			++c;
		}
	}
	k = 0;
	for (int i = 0; i < allKeys; ++i) {
		comparator[c].comp[k][0] = compsCHART_OFF_1[i];
		comparator[c].comp[k][1] = compsCHART_OFF_2[i];
		++k;
		if (k > 6) {
			k = 0;
			++c;
		}
	}
}

void start_chart() {
	if (fl == 1) {
		lv_chart_set_point_count(objects.chart, allKeys);

		ser_1 = lv_chart_add_series(objects.chart,
			lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
		ser_2 = lv_chart_add_series(objects.chart,
			lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_SECONDARY_Y);
		lv_chart_set_axis_range(objects.chart, LV_CHART_AXIS_PRIMARY_Y,
			on_green_min, on_green_max);
		lv_chart_set_axis_range(objects.chart, LV_CHART_AXIS_SECONDARY_Y,
			on_red_min, on_red_max);
		c1 = lv_chart_add_cursor(objects.chart, lv_color_make(250, 250, 250),
			LV_DIR_VER);
		lv_chart_set_cursor_point(objects.chart, c1, ser_1, cursor);
		lv_obj_set_style_line_width(objects.chart, 0, LV_PART_ITEMS); // толщина линий на графике
		lv_obj_set_style_size(objects.chart, 2, 3, LV_PART_INDICATOR); // размер точек на графике
		lv_obj_add_state(objects.s1_s2_on, 16);
		lv_obj_add_state(objects.s1_s2_off, 16);
		lv_obj_add_state(objects.s1_s2_button, 16);
		lv_obj_add_state(objects.s1_s2_button_1, 16);
		fl = 0;
	}
}

void check_max_min() {
	for (int i = 0; i < allKeys; ++i) {
		if (m_m.on.s1.min > compsCHART_ON_1[i]) {
			m_m.on.s1.min = compsCHART_ON_1[i];
		}
		if (m_m.on.s1.max < compsCHART_ON_1[i]) {
			m_m.on.s1.max = compsCHART_ON_1[i];
		}
		if (m_m.on.s2.min > compsCHART_ON_2[i]) {
			m_m.on.s2.min = compsCHART_ON_2[i];
		}
		if (m_m.on.s2.max < compsCHART_ON_2[i]) {
			m_m.on.s2.max = compsCHART_ON_2[i];
		}
		if (m_m.off.s1.min > compsCHART_OFF_1[i]) {
			m_m.off.s1.min = compsCHART_OFF_1[i];
		}
		if (m_m.off.s1.max < compsCHART_OFF_1[i]) {
			m_m.off.s1.max = compsCHART_OFF_1[i];
		}
		if (m_m.off.s2.min > compsCHART_OFF_2[i]) {
			m_m.off.s2.min = compsCHART_OFF_2[i];
		}
		if (m_m.off.s2.max < compsCHART_OFF_2[i]) {
			m_m.off.s2.max = compsCHART_OFF_2[i];
		}
	}
	s1_on_min.clear();
	s1_on_min = std::to_string(m_m.on.s1.min);
	s1_on_max.clear();
	s1_on_max = std::to_string(m_m.on.s1.max);
	s2_on_min.clear();
	s2_on_min = std::to_string(m_m.on.s2.min);
	s2_on_max.clear();
	s2_on_max = std::to_string(m_m.on.s2.max);
	s1_off_min.clear();
	s1_off_min = std::to_string(m_m.off.s1.min);
	s1_off_max.clear();
	s1_off_max = std::to_string(m_m.off.s1.max);
	s2_off_min.clear();
	s2_off_min = std::to_string(m_m.off.s2.min);
	s2_off_max.clear();
	s2_off_max = std::to_string(m_m.off.s2.max);
}

void char_correction(int x, plus_minus pm) {

	if (lv_scr_act() == objects.chart_manual_edit_on_disp) {
		if (fl_on == 1)
			pm == plus_minus::plus ?
			compsCHART_ON_1[cursor] += x : compsCHART_ON_1[cursor] -= x;
		if (fl_on == 2)
			pm == plus_minus::plus ?
			compsCHART_ON_2[cursor] += x : compsCHART_ON_2[cursor] -= x;
	}
	else if (lv_scr_act() == objects.chart_manual_edit_off_disp) {
		if (fl_off == 1)
			pm == plus_minus::plus ?
			compsCHART_OFF_1[cursor] += x : compsCHART_OFF_1[cursor] -=
			x;
		if (fl_off == 2)
			pm == plus_minus::plus ?
			compsCHART_OFF_2[cursor] += x : compsCHART_OFF_2[cursor] -=
			x;
	}
	else if (lv_scr_act() == objects.chart_graph_resize) {
		if (fl_disp) { // 0 = on, 1 = off
			if (s1_s2) { // 0 = green, 1 = red
				if (top_bot) { // 0 = top, 1 = bottom
					pm == plus_minus::minus ? off_red_min += x : off_red_min -=
						x;
				}
				else {
					pm == plus_minus::minus ? off_red_max += x : off_red_max -=
						x;
				}
			}
			else {
				if (top_bot) {
					pm == plus_minus::minus ?
						off_green_min += x : off_green_min -= x;
				}
				else {
					pm == plus_minus::minus ?
						off_green_max += x : off_green_max -= x;
				}
			}
			lv_chart_set_axis_range(objects.chart, LV_CHART_AXIS_PRIMARY_Y,
				off_green_min, off_green_max);
			lv_chart_set_axis_range(objects.chart, LV_CHART_AXIS_SECONDARY_Y,
				off_red_min, off_red_max);
			s1_on_min.clear();
			s1_on_min = std::to_string(off_green_min);
			s1_on_max.clear();
			s1_on_max = std::to_string(off_green_max);
			s2_on_min.clear();
			s2_on_min = std::to_string(off_red_min);
			s2_on_max.clear();
			s2_on_max = std::to_string(off_red_max);
		}
		else {
			if (s1_s2) {
				if (top_bot) {
					pm == plus_minus::minus ? on_red_min += x : on_red_min -= x;
				}
				else {
					pm == plus_minus::minus ? on_red_max += x : on_red_max -= x;
				}
			}
			else {
				if (top_bot) {
					pm == plus_minus::minus ?
						on_green_min += x : on_green_min -= x;
				}
				else {
					pm == plus_minus::minus ?
						on_green_max += x : on_green_max -= x;
				}
			}
			lv_chart_set_axis_range(objects.chart, LV_CHART_AXIS_PRIMARY_Y,
				on_green_min, on_green_max);
			lv_chart_set_axis_range(objects.chart, LV_CHART_AXIS_SECONDARY_Y,
				on_red_min, on_red_max);
			s1_on_min.clear();
			s1_on_min = std::to_string(on_green_min);
			s1_on_max.clear();
			s1_on_max = std::to_string(on_green_max);
			s2_on_min.clear();
			s2_on_min = std::to_string(on_red_min);
			s2_on_max.clear();
			s2_on_max = std::to_string(on_red_max);
		}
		return;
	}
	chart_to_comp();
	on_off_s1_s2_min_max mm; //  для сброса состояния max_min
	m_m = mm;
	check_max_min();
	lv_chart_refresh(objects.chart);
}

