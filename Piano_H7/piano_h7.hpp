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

	std::string ch_o;
	std::string ch_f;
	std::string s1_on_min;
	std::string s1_on_max;
	std::string s2_on_min;
	std::string s2_on_max;
	std::string s1_off_min;
	std::string s1_off_max;
	std::string s2_off_min;
	std::string s2_off_max;
	std::string sensor_on_1_data_string;
	std::string sensor_on_2_data_string;
	std::string sensor_off_1_data_string;
	std::string sensor_off_2_data_string;
	std::string divisible_eez_string;
	std::string cursor_string;
	std::string disp_on_off_button;
	std::string disp_on_off_button_3;
	std::string top_bot_str;
	std::string top_bot_str_2;
	std::string debugg;  // DEBUG
	std::string chart_calib_online;
	std::string l;
	std::string r;




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

	enum class dot {
		green = 0,
		red = 1,
		grey
	};

	enum class plus_minus {
		plus,
		minus,
		none
	};

	enum class current_display {
		on,
		off,
		d_none,
		dis_main,
		d_flash
	};

	enum class color_but {
		green,
		red,
		c_none
	};

	enum class but_top_bot {
		top,
		bot,
		t_none
	};

	enum class calib_all_on_off {
		calib_on,
		calib_off,
		calib_none
	};

	enum class response {
		ok = 0x0C,
		fail = 0xDD
	};

	enum class state { // TODO deprecated
		bootloader = 1,
		piano
	};

	enum class command { // TODO G4 обновить код
		sync_timer = 1,
		cal,
		read_comp_value, // TODO добавить проверки (принять ответ)
		set_comp_value, // TODO добавить проверки (принять ответ)
		read_comp_setting, // TODO реализовать
		all_calib,
		reset_to_bootloader,
		mute, // TODO реализовать (chips ON - mute)
		unmute, // TODO реализовать
		echo = 11 // TODO реализовать для основной прошивки
	};

	enum class subcommand {
		start_calibration = 1,
		read_calibration,
		stop_calibration,
		working,
		nothing
	};

	enum class bootloader_command {
		echo = 11,
		reset, // 200ms delay // TODO надо реализовать
		data_from_H7_to_array_g4,
		copy_array_to_flash_g4,
		jump_to_piano_g4, // прыжок по зашитому в g4 адресу
		jump_g4_to_adress // TODO сделать // прыжок по указанному адресу
	};

	enum class chip_states {
		boot = 1,
		main
	};

	enum class typeAction {
		on = 1,
		off
	};

	struct cursor_min_max {
		uint8_t cursor_on_min = 0;
		uint8_t cursor_on_max = 0;
		uint8_t cursor_off_min = 0;
		uint8_t cursor_off_max = 0;
	};

	struct comparator {
		uint8_t cursor = 0;
		uint8_t address = 0;
		uint8_t number_chip = 0;
		uint8_t number_comparator = 0;
		bool    is_active = false;
	};

	struct Chip {
		uint8_t number_chip = 0; // переделать в просто "number"
		std::vector<comparator> comparators; // переделать в "comps"
		typeAction typ = typeAction::on;
		chip_states st = chip_states::boot;
	};

		// номер 95 у последней верхней клавиши
		// ***** 390(263)-14000(22723)us пролёт молоточка

	const uint32_t allChipCount = 27; // 1-13-on, 14-23(26)-off // до этого значения считает таймер // TODO int->uint32_t ?? в 449й строке сохранение в память потому-что! И надо ставить на один больше, чем фактически? 

	const uint8_t start_adress_chip_on = 5; // включительно (1)
	const uint8_t end_adress_chip_on = 7; // включительно (13) (если < start_adress_chip_on, то выключено) // TODO проверить этот момент..
	const uint8_t start_adress_chip_off = 14; // включительно (14)
	const uint8_t end_adress_chip_off = 10; // включительно всего (23)

	const uint32_t start_cursor = 35;
	volatile uint32_t cursor = start_cursor; // TODO volatile?

	uint8_t rx_data[4] = { };
	const uint32_t dataLengthRX = sizeof(rx_data);
	__attribute__((aligned(8))) uint8_t tx_settings[5] = { };
	const uint8_t tx_settings_length = sizeof(tx_settings);
	__attribute__((aligned(8))) uint8_t rx_settings[5] = { };
	const uint8_t rx_settings_length = sizeof(rx_settings);
	uint8_t compN_ = 0;
	uint8_t dot_ = 0;
	uint8_t a_ = 0;
	uint8_t b_ = 0;
	state ship_is = state::bootloader;

	int32_t def_on[2] = { 2600, 1000 }; // [0]-green, [1]-red
	int32_t def_off[2] = { 2100, 2400 }; // [0]-green, [1]-red

	const int32_t sizeCHART_BUFFER = 196;
	int32_t compsCHART_green[sizeCHART_BUFFER] = {}; // green // TODO vector переделать это в vector
	int32_t compsCHART_red[sizeCHART_BUFFER] = {}; // red
	int32_t compsCHART_CALIB[sizeCHART_BUFFER] = {};
	int32_t compsCHART_CALIB_old[sizeCHART_BUFFER] = {};

	std::vector<int> numbers_chips; // TODO deprecated



	// --()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()
	// new variants

	static const uint32_t ADRESS_H7_BOOTLOADER = 0x08000000;
	static const uint32_t ADRESS_H7_MAIN_FIRMWARE = 0x08020000; // размер +- 0x0008E064 до ~0x080AE070 до 5го блока включительно
	static const uint32_t ADRESS_H7_MAIN_FIRMWARE_FOR_G4 = 0x080C0000; // хватает ли места для размещения прошивки? (6й блок) ~0x2f40 размер
	// надо 6 копирований делать в G4                    ^^^^^^^^^^^^^
	static const uint32_t FLASH_ADDRESS = 0x080E0000; // -здесь лежит калибровка

	static const uint32_t ADRESS_G4_CHIP_NUMBER = 0x08003800; // здесь храним номер чипа (в памяти g4) 7я банка
	static const uint32_t ADRESS_G4_MAIN_FIRMWARE = 0x08008000; // этот адрес зашит в памяти g4 (16я банка) - сейчас размер на 7 банок.
	static const uint32_t ADRESS_G4_MAIN_FIRMWARE_ALT = 0x08008000; // здесь меняем куда шить и куда прыгать
	static const uint32_t COUNT_PAGE_FOR_FIRMWARE_G4 = 7;// количество страниц (7) в g4, которые занимает прошивка g4 (16-22)

	const uint32_t size_BUFFER = 200; // чётное
	const uint32_t buffer_division = size_BUFFER / 2;
	const uint32_t on_off_division = 14; // точка разделения on-off

	int32_t buffer_green[size_BUFFER] = {};
	int32_t buffer_red[size_BUFFER] = {};
	int32_t buffer_calib[size_BUFFER] = {};
	int32_t buffer_calib_old[size_BUFFER] = {};

	int32_t green_on_default = 2601;
	int32_t red_on_default = 1001;
	int32_t green_off_default = 2102;
	int32_t red_off_default = 2402;

	std::vector<Chip> vChips;
	std::map<uint8_t, comparator> mComparatorCursor_on;
	std::map<uint8_t, comparator> mComparatorCursor_off;
	static comparator default_comparator; // для вывода из searcher_addr_in_cursor() когда нет объекта для возврата по ссылке

	const uint8_t count_comparators = 7;
	chip_states chip_state = chip_states::boot; // TODO переименовать, когда удалю class state
	cursor_min_max c_mi_ma; // TODO реализовать мин-макс

	// --()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()

	int fl = 0; // for test fl

	float timerLenght_F = 0; // for test
	float speed_F = 0; // for test
	float energy_F = 0; // for test
	float midi_hi_F = 0; // for test
	float midi_lo_F = 0; // for test
	uint32_t debug_counter = 0;
	int tt1 = 0;
	int tt2 = 0;
	int tt3 = 0;
	float timer_data_in = 0;

	int8_t noteAdder[size_BUFFER] = {};
	float mass_F[size_BUFFER] = {}; // TODO FLASH сделать сохранениее в память


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

	volatile int touchpad_pressed = 0; // TODO deprecated?
	int touchpad_x = 0; // TODO deprecated?
	int touchpad_y = 0; // TODO deprecated?

	current_display cur_disp = current_display::d_none;
	color_but col_but = color_but::c_none;
	but_top_bot top_bot = but_top_bot::t_none;
	calib_all_on_off calib_all_OnOff = calib_all_on_off::calib_none;

	void pwr();
	void to_sleep();
	void init();
	void init_LL();
	void init_LCD_touch();
	void disp_start();
	void touch_start();
	void init_chips();
	void h7();
	void sync();
	int sync_sender(const uint8_t& i);
	void initBuffers();
	void check_max_min();
	void all_H7_to_g4();
	void all_g4_to_H7();
	void sender(const command& com, const uint8_t& adress, const uint8_t& compN, const dot& dot, const uint32_t& value);
	void UART4_SendAddress(const uint8_t& slave_address);
	void UART4_Send_Settings(const command& com, const uint8_t& compN, const uint8_t& dot, const uint32_t& value);
	void UART4_Receive_Settings();
	void UART4_Send_Settings_bootloader();
	void UART4_Receive_Settings_bootloader();
	void DMA1_RX();
	void USART_Noise_Error_detected();
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
	void G4_echo();
	void reset_bootloaders();
	void Set_tx_s(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e);
	int UART4_Receive_timeout_10us();

	void my_input_read(lv_indev_t* indev, lv_indev_data_t* data);
	void my_flush_cb(lv_display_t* disp, const lv_area_t* area, uint16_t* color_p);

	// void my_flush_wait(lv_display_t* disp);

	void DMA2_Stream1_TransferComplete();
	void DMA2_Stream3_i2c();

	void configCharts();

	void Read_uint32(uint32_t Address, volatile uint32_t* pData, uint32_t Size);
	// void Flash_uint32(uint32_t Address, volatile uint32_t* Data, uint32_t size);
	static inline void uint32_to_bytes_pointer(uint32_t value, uint8_t* bytes);
	static inline uint32_t bytes_to_uint32_pointer(const uint8_t* bytes);
	void data_from_H7_to_g4();
	void flash_g4(const uint32_t& addr, const int& chip_number);
	void jump_g4s_to_adress();
	void reset_main_to_bootloader();
	void refresh_cursor(const Chip& comp);
	const comparator& searcher_addr_in_cursor(const uint32_t& chip);


	lv_display_t* disp;
	lv_indev_t* indev;

	lv_obj_t* cur_shart;
	lv_chart_series_t* ser_on_green;
	lv_chart_series_t* ser_on_red;
	lv_chart_series_t* ser_on_blue;
	lv_chart_cursor_t* cursor_on_vert;
	lv_chart_cursor_t* cursor_on_hor;
	lv_style_t cursor_style;
	lv_chart_series_t* ser_off_green;
	lv_chart_series_t* ser_off_red;
	lv_chart_series_t* ser_off_blue;
	lv_chart_cursor_t* cursor_off_vert;
	lv_chart_cursor_t* cursor_off_hor;

	std::string test_t_out_fl; // for test
	std::string test_speed_fl; // for test
	std::string test_energy_fl; // for test
	std::string test_midi_hi_fl; // for test
	std::string test_midi_lo_fl; // for test
	std::string note; // for test
	std::string mass_str;
	std::string tim_str_in; // for test
	std::string t1; // for test
	std::string t2; // for test
	std::string t3; // for test
	std::string timer_data; // for test


	std::string test_timer2; // for test
	std::string calib_all_str;
	volatile uint32_t test_int_timer2 = 0; // for test
	volatile uint32_t oldCursor = start_cursor;
	float mass_to_disp = 0; // for test

	// настройки gpio для DISPLAY взяты отсюда: https://github.com/zeruns/STM32F407_LVGL_Template_MSP3526/blob/master/Core/Src/gpio.c
	const uint32_t BYTES_PER_PIXEL = (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB888)); // TODO 565 or 888? (было 565)
	const uint32_t BUFF_SIZE = (480 * 20 * BYTES_PER_PIXEL);
	static lv_color16_t buf_1[BUFF_SIZE]; // TODO 16 or 8
	static lv_color16_t buf_2[BUFF_SIZE];




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
