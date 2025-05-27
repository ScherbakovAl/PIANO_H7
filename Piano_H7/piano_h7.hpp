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
#include "ui.h"
#include "tusb.h"

#ifdef __cplusplus
extern "C" {

// #include <string>

	using uint = unsigned int;
	using cuint = const uint;

	const int allChipCount = 26;
	//const int chipAdress = 1; // для NoteOff что-то придумать надо здесь!
	// ***** 390(263)-14000(22723)us пролёт молоточка
	const int allKeys = allChipCount / 2 * 7;

	uint8_t rx_data[3] = { };
	const int dataLengthRX = sizeof(rx_data);
	uint8_t tx_settings[5] = { };
	const int tx_settings_length = sizeof(tx_settings);
	uint8_t rx_settings[5] = { };
	const int rx_settings_length = sizeof(rx_settings);
	uint8_t compN_ = 0;
	uint8_t dot_ = 0;
	uint8_t a_ = 0;
	uint8_t b_ = 0;
	int f = 0;
	cuint Flash_Address = 0x08040000;
	cuint key_to_change_memory[8] = { 0xBAFC }; // 0x640 - смещение

	struct comps {
		int comp[8][2] = { {3500, 1005}, {3501, 1000}, {3502, 1001}, {3503, 1000}, {3504, 1000}, {3505, 1000}, {3506, 1000}, {0, 0} };
	};

	int def[2] = { 3500, 1000 };

	struct conv_16_8 {
		uint8_t a = 0;
		uint8_t b = 0;
	};

	struct min_max {
		int max = 0;
		int min = 4095;
	};

	struct s1s2 {
		min_max s_green;
		min_max s_red;
	};

	struct on_off_s1_s2_min_max {
		s1s2 on;
		s1s2 off;
	};

	enum command {
		sync_timer = 1,
		cal,
		read_comp_value,
		set_comp_value,
		//set_div_value, // реализовать // TODO
		buff_to_flash,         //
		flash_to_buff,         //
		read_comp_value_flash, //
	};

	enum plus_minus {
		plus,
		minus,
		none
	};

	enum current_display {
		on,
		off,
		d_none
	};

	enum color_but {
		green,
		red,
		c_none
	};

	enum but_top_bot {
		top,
		bot,
		t_none
	};

	comps comparator[allChipCount];
	comps test[allChipCount]; // for test flash
	comps test_in[allChipCount]; // for test flash

	on_off_s1_s2_min_max m_m;
	int32_t compsCHART_ON_1[allKeys] = {};
	int32_t compsCHART_ON_2[allKeys] = {};
	int32_t compsCHART_OFF_1[allKeys] = {};
	int32_t compsCHART_OFF_2[allKeys] = {};
	int on_green_max = 4600;
	int on_green_min = 0;
	int on_red_max = 4600;
	int on_red_min = 0;
	int off_green_max = 3514;
	int off_green_min = 3494;
	int off_red_max = 1011;
	int off_red_min = 991;
	int divis = 1000000000;
	volatile int touchpad_pressed = 0;
	int touchpad_x = 0;
	int touchpad_y = 0;
	int32_t cursor = 28;
	current_display cur_disp = d_none;
	color_but col_but = c_none;
	but_top_bot top_bot = t_none;

	void h7();
	void UART4_SendAddress(const uint8_t& slave_address);
	void UART4_Send_Settings(const command& com, const uint8_t& compN, const uint8_t& dot, const int& value);
	void UART4_Receive_Settings();
	conv_16_8 convert_16_8(const int& a);
	int convert_8_16(const uint8_t& a, const uint8_t& b);
	void pause(const int& p);
	void sync();
	void calibration(const uint8_t& adress, const uint8_t& compN, const uint8_t& dot);
	void readCompValue(const uint8_t& adress, const uint8_t& compN, const uint8_t& dot);
	void setCompValue(const uint8_t& adress, const uint8_t& compN, const uint8_t& dot, const int& value);
	void sender(const command& com, const uint8_t& adress, const uint8_t& compN, const uint8_t& dot, const int& value);
	void SaveToMemory();
	void ReadOnMemory();
	void DMA1_RX();
	void DMA2_Stream3_TransferComplete();
	void send_test_midi();
	void my_flush_cb(lv_display_t* disp, const lv_area_t* area, uint16_t* color_p);
	void my_input_read(lv_indev_t* indev, lv_indev_data_t* data);
	void config_charts();
	void comp_to_chart();
	void chart_to_comp();
	void chart_correction(const int& x, const plus_minus& pm);
	void check_max_min();
}
#endif // extern "C"

//		GPIOC->BSRR = 0x200; // pC9
//		GPIOC->BSRR = 0x2000000; // pC9
//		GPIOC->BSRR = 0x80; // pC7
//		GPIOC->BSRR = 0x800000; // pC7
//		GPIOD->BSRR = 0x8000; // pD15
//		GPIOD->BSRR = 0x80000000; // pD15
//		GPIOD->BSRR = 0x2000; // pD13
//		GPIOD->BSRR = 0x20000000; // pD13
//		GPIOD->BSRR = 0x800; // pD11
//		GPIOD->BSRR = 0x8000000; // pD11
//		GPIOD->BSRR = 0x200; // pD9
//		GPIOD->BSRR = 0x2000000; // pD9
//		GPIOB->BSRR = 0x8000; // pB15
//		GPIOB->BSRR = 0x80000000; // pB15
//		GPIOB->BSRR = 0x2000; // pB13
//		GPIOB->BSRR = 0x20000000; // pB13
//		GPIOE->BSRR = 0x8; // pE3 LED
//		GPIOE->BSRR = 0x80000; // pE3 LED

/*TX*/
/* Clean D-cache */
/* Make sure the address is 32-byte aligned and add 32-bytes to length, in case it overlaps cacheline */
/*SCB_CleanDCache_by_Addr((uint32_t*)(((uint32_t)tx_buffer) & ~(uint32_t)0x1F), TX_LENGTH+32);*/
//			SCB_CleanDCache_by_Addr((uint32_t*)(((uint32_t)tx_data) & ~(uint32_t)0x1F), 3); // когда включениы ICache & DCache это необходимо использовать
//			LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_1);
/*RX*/
/* Invalidate D-cache before reception */
/* Make sure the address is 32-byte aligned and add 32-bytes to length, in case it overlaps cacheline */
/*SCB_InvalidateDCache_by_Addr((uint32_t*)(((uint32_t)rx_buffer) & ~(uint32_t)0x1F), RX_LENGTH+32);*/
/* No access to rx_buffer should be made before DMA transfer is completed */
