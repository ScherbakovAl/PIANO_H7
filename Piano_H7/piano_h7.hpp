/*
 * midi_keyboard_h7-2.h
 *
 *  Created on: Apr 8, 2025
 *      Author: sche
 */

#pragma once
#include "st7796.h"
#include "ft6336.h"
#include "lvgl.h"


#ifdef __cplusplus
extern "C" {

	//#include "deque"
	//using sdfg = int;

	const int allChipCount = 26;
	const int allKeys = allChipCount / 2 * 7;
	int32_t compsCHART_ON_1[allKeys] = { };
	int32_t compsCHART_ON_2[allKeys] = { };
	int32_t compsCHART_OFF_1[allKeys] = { };
	int32_t compsCHART_OFF_2[allKeys] = { };
	int on_green_max = 3515;
	int on_green_min = 3495;
	int on_red_max = 1010;
	int on_red_min = 990;
	int off_green_max = 3514;
	int off_green_min = 3494;
	int off_red_max = 1011;
	int off_red_min = 991;
	int divis = 1000000000;
	int s1_s2 = 1;
	int top_bot = 0;

	void h7();
	void my_flush_cb(lv_display_t* disp, const lv_area_t* area, uint16_t* color_p);
	void my_input_read(lv_indev_t* indev, lv_indev_data_t* data);
	void manual_edit_on();
	void manual_edit_off();
	volatile int touchpad_pressed = 0;
	int touchpad_x = 0;
	int touchpad_y = 0;
	int32_t cursor = 10;
	int fl_on = 0;
	int fl_off = 0;
	int fl_disp = 0;

	struct comps {
		int comp[8][2] = { { 3500, 1005 }, { 3501, 1000 }, { 3502, 1001 }, { 3503, 1000 }, { 3504, 1000 }, { 3505, 1000 }, { 3506, 1000 }, { 0, 0 } };
	};
	int def[2] = { 3500, 1000 };

	struct min_max {
		int max = 0;
		int min = 4095;
	};
	struct s1s2 {
		min_max s1;
		min_max s2;
	};
	struct on_off_s1_s2_min_max {
		s1s2 on;
		s1s2 off;
	};
	on_off_s1_s2_min_max m_m;
	void check_max_min();

	comps comparator[allChipCount];

	void comp_to_chart();
	void chart_to_comp();
	void start_chart();
	enum plus_minus {
		plus, minus
	};

	void char_correction(int x, plus_minus pm);
}
#endif // extern "C"

//		GPIOC->BSRR = 0x200; // pC9
//		GPIOC->BSRR = 0x2000000; // pC9
//		GPIOC->BSRR = 0x80; // pC7
//		GPIOC->BSRR = 0x800000; // pC7
