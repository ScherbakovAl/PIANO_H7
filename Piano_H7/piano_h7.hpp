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
#include "vars.h"
#include "actions.h"

#ifdef __cplusplus
extern "C" {

	using uint = unsigned int;
	using cuint = const uint;

	struct conv_16_8 {
		uint8_t a = 0;
		uint8_t b = 0;
	};

	struct min_max {
		int32_t max = 0;
		int32_t min = 4095;
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
		buff_to_flash,
		flash_to_buff,
		read_comp_value_flash,
		all_calib
	};

	enum subcommand {
		start_calibration = 1,
		read_calibration,
		stop_calibration,
		working,
		nothing
	};

	enum plus_minus {
		plus,
		minus,
		none
	};

	enum current_display {
		on,
		off,
		d_none,
		dis_main
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

	enum calib_all_on_off {
		calib_on,
		calib_off,
		calib_none
	};


	// пины тестовой колодки
	// pin 2 - tim trig
	// pin 4 - tim slc
	// pin 6 - Urx
	// pin 8 - Utx

	// TODO сделать проверку: сколько раз заходит в циклы While при работе с uart?

	// TODO проверить UART TX должен быть подтянут к UP?

	// номер 95 у последней верхней клавиши
	// ***** 390(263)-14000(22723)us пролёт молоточка

	const int allChipCount = 27; // 1-13-on, 14-23(26)-off // до этого значения считает таймер // TODO int->uint32_t ?? в 449й строке сохранение в память потому-что! И надо ставить на один больше, чем фактически? 

	const uint8_t start_adress_chip_on = 8; // включительно
	const uint8_t end_adress_chip_on = 10; // включительно (если < start_adress_chip_on, то выключено) // TODO проверить этот момент..
	const uint8_t start_adress_chip_off = 25; // включительно
	const uint8_t end_adress_chip_off = 23; // включительно всего 23

	const uint32_t start_cursor = 70;
	volatile uint32_t cursor = start_cursor;

	uint8_t rx_data[4] = { };
	const uint32_t dataLengthRX = sizeof(rx_data);
	uint8_t tx_settings[5] = { };
	const uint8_t tx_settings_length = sizeof(tx_settings);
	uint8_t rx_settings[5] = { };
	const uint8_t rx_settings_length = sizeof(rx_settings);
	uint8_t compN_ = 0;
	uint8_t dot_ = 0;
	uint8_t a_ = 0;
	uint8_t b_ = 0;
	// int f = 0; // TODO используеся где?

	int32_t def_on[2] = { 2600, 1000 }; // [0]-green, [1]-red
	int32_t def_off[2] = { 2100, 2400 }; // [0]-green, [1]-red

	const int32_t sizeCHART_BUFFER = 196;
	int32_t compsCHART_0[sizeCHART_BUFFER] = {}; // green
	int32_t compsCHART_1[sizeCHART_BUFFER] = {}; // red
	int32_t compsCHART_CALIB[sizeCHART_BUFFER] = {};
	int32_t compsCHART_CALIB_old[sizeCHART_BUFFER] = {};

	int8_t noteAdder[196] = {};
	float mass_F[196] = {};

	const uint32_t Flash_Address = 0x080E0000; // FLASH

	 // TODO vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv TODO int->uint32_t ?? в 449й строке сохранение в память потому-что! 
	const uint32_t key_to_change_memory = { 0xBAFC }; // allChipCount * 0x40 - смещение; 0xBAFC - просто код, который если изменить, то данные перезапишутся в памяти

	on_off_s1_s2_min_max m_m;
	int32_t on_green_max = 4095;
	int32_t on_green_min = 0;
	int32_t on_red_max = 4095;
	int32_t on_red_min = 0;
	int32_t off_green_max = 4095;
	int32_t off_green_min = 0;
	int32_t off_red_max = 4095;
	int32_t off_red_min = 0;
	int divis = 100'000'000;

	volatile int touchpad_pressed = 0;
	int touchpad_x = 0;
	int touchpad_y = 0;

	current_display cur_disp = d_none;
	color_but col_but = c_none;
	but_top_bot top_bot = t_none;
	calib_all_on_off calib_all_OnOff = calib_none;

	void h7();
	void sync();
	int sync_sender(const uint8_t& i);
	void initBuffers();
	void check_max_min();
	void all_H7_to_g4();
	void all_g4_to_H7();
	void checkDataOnSensor(const uint8_t& adress);
	void sender(const command& com, const uint8_t& adress, const uint8_t& compN, const uint8_t& dot, const uint32_t& value);
	void UART4_SendAddress(const uint8_t& slave_address);
	void UART4_Send_Settings(const command& com, const uint8_t& compN, const uint8_t& dot, const uint32_t& value);
	void UART4_Receive_Settings();
	void DMA1_RX();
	void DMA_UART_ERRORS_HANDLER();
	int32_t convert_8_16(const uint8_t& a, const uint8_t& b);
	conv_16_8 convert_16_8(const uint32_t& a);
	void chart_correction(const uint32_t& x, const plus_minus& pm);
	void SaveToMemory();
	void ReadOnMemory();
	void pause(const uint32_t& p);
	void debugg_fn(const std::string& str);  // DEBUG
	void debugg_clear();
	void resetPin();
	void send_test_midi();

	void my_input_read(lv_indev_t* indev, lv_indev_data_t* data);
	void my_flush_cb(lv_display_t* disp, const lv_area_t* area, uint16_t* color_p);

	void configCharts();
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
