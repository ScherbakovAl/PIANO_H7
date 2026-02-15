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

	enum class dot {
		green = 0,
		red = 1,
		grey
	};

	enum class current_display {
		on,
		off,
		none,
		main,
		bootloader
	};

	enum class response {
		ok = 0x0C,
		fail = 0xDD
	};

	enum class command {
		sync_timer = 1,
		cal,
		read_comp_value,
		set_comp_value,
		read_comp_setting, // TODO реализовать
		all_calib,
		reset_to_bootloader,
		mute,
		unmute,
		echo = 11
	};

	enum class subcommand {
		start_calibration = 1,
		read_calibration,
		stop_calibration,
		working,
		nothing,
	};

	enum class bootloader_command {
		echo = 11,
		reset, // 200ms delay
		data_from_H7_to_array_g4,
		copy_array_to_flash_g4,
		jump_to_piano_g4, // прыжок по зашитому в g4 адресу
		jump_g4_to_adress // прыжок по указанному адресу
	};

	enum class chip_states {
		boot = 1,
		main,
		none
	};

	enum class typeAction {
		on = 1,
		off,
		none
	};

	struct conv_16_8 {
		uint8_t a = 0;
		uint8_t b = 0;
	};

	struct comparator {
		uint8_t cursor = 0;
		uint8_t address = 0;
		uint8_t number_chip = 0;
		uint8_t number_comparator = 0;
		bool    is_active = false;
	};

	struct Chip {
		uint8_t number_chip = 0;
		std::vector<comparator> comparators;
		typeAction typ = typeAction::none;
		chip_states st = chip_states::none;
	};

	struct speed_for_midi {
		const float key_mass = 0.0f; // 80000 == 80 gr
		const float distance = 0.0f; // 1700 == 1.7 mm
		const float offset_x = 0.0f; // offset x
		const float offset_y = 0.0f; // offcet y
	};


	// ***** 390(263)-14000(22723)us пролёт молоточка
	uint8_t rx_data[4] = { };
	const uint32_t dataLengthRX = sizeof(rx_data);
	__attribute__((aligned(8))) uint8_t tx_settings[5] = { };
	const uint32_t tx_settings_length = sizeof(tx_settings);
	__attribute__((aligned(8))) uint8_t rx_settings[5] = { };
	const uint32_t rx_settings_length = sizeof(rx_settings);
	uint8_t compN_ = 0;
	uint8_t dot_ = 0;
	uint8_t a_ = 0;
	uint8_t b_ = 0;

	// new variants
	// --()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()

	static const uint32_t ADDRESS_H7_BOOTLOADER = 0x08000000;
	static const uint32_t ADDRESS_H7_MAIN_FIRMWARE = 0x08020000; // размер +- 0x0008E064 до ~0x080AE070 до 5го блока включительно
	static const uint32_t ADDRESS_H7_MAIN_FIRMWARE_FOR_G4 = 0x080C0000; // хватает ли места для размещения прошивки? (6й блок) ~0x2f40 размер
	static const uint32_t ADDRESS_H7_CALIB = 0x080E0000; // -здесь лежит калибровка

	static const uint32_t ADDRESS_G4_CHIP_NUMBER = 0x08003800; // здесь храним номер чипа (в памяти g4) 7я банка
	static const uint32_t ADDRESS_G4_MAIN_FIRMWARE = 0x08008000; // этот адрес зашит в памяти g4 (16я банка) - сейчас размер на 7 банок.
	static const uint32_t ADDRESS_G4_MAIN_FIRMWARE_ALT = 0x08008000; // здесь меняем куда шить и куда прыгать
	static const uint32_t COUNT_PAGE_FOR_FIRMWARE_G4 = 7;// количество страниц (7) в g4, которые занимает прошивка g4 (16-22)


	const uint32_t allChipCount = 27; // 1-13-on, 14-23(26)-off // до этого значения считает таймер 
	const uint32_t on_off_division = 14; // точка разделения on-off

	const uint32_t size_BUFFER = 200; // чётное
	const uint32_t buffer_division = size_BUFFER / 2;
	const uint32_t count_comparators = 7;

	int32_t buffer_green[size_BUFFER] = {};
	int32_t buffer_red[size_BUFFER] = {};
	int32_t buffer_calib[size_BUFFER] = {};
	int32_t buffer_calib_old[size_BUFFER] = {};
	int32_t buffer_dac[size_BUFFER] = {};

	int32_t green_on_default = 2601;
	int32_t red_on_default = 1001;
	int32_t green_off_default = 2102;
	int32_t red_off_default = 2402;

	std::vector<Chip> vChips;
	std::map<uint8_t, comparator> mComparatorCursor_on;
	std::map<uint8_t, comparator> mComparatorCursor_off;
	static comparator default_comparator; // для вывода из searcher_addr_in_cursor() когда нет объекта для возврата по ссылке

	chip_states chip_state = chip_states::none; // TODO переименовать, когда удалю class state
	current_display cur_disp = current_display::none;
	volatile uint32_t cursor = 0; // TODO volatile?
	uint32_t cursor_offset_on = 0;
	uint32_t cursor_offset_off = 0;

	int8_t noteAdder[size_BUFFER] = {};
	float mass_F[size_BUFFER] = {}; // TODO FLASH сделать сохранениее в память

	// --()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()

	int fl = 0.0f; // for test fl
	float speed_F = 0.0f; // for test
	float energy_F = 0.0f; // for test
	float midi_hi_F = 0.0f; // for test
	float midi_lo_F = 0.0f; // for test
	uint32_t out_debug_ = 0; // for test
	float timer_data_in = 0.0f;
	uint32_t debug_counter = 0.0f;
	const uint32_t key_to_change_memory = { 0xBAFC }; // allChipCount * 0x40 - смещение; 0xBAFC - просто код, который если изменить, то данные перезапишутся в памяти

	float min = 3800.0f; // 3261 (3834) (~3600)
	float max = 181987.0f; // 170106 (170000) 181000


	const float key_mass = 0.008f; // 8 гр -->> переехал в массив
	// const float distance_F = 0.0017f; // 1.7 мм (толщина шаблонов 1.9 и 0.2)
	// const float div_on = 0.000000000092f; // меньше - громче
	// const float div_off = 0.00000000004f; // меньше - громче 
	const float deriv_F = 2.0f; // делить на 2 в формуле
	const float maxMidi_F = 127.99f;
	std::vector<speed_for_midi> speeds;
	std::vector<speed_for_midi> speeds_ON;
	float m_F = 0.0f; // for test
	float sd_F = 0.0f; // for test
	float sx_F = 0.0f; // for test
	float sy_F = 0.0f; // for test

	void pwr();
	void to_sleep();
	void init();
	void init_LL();
	void init_LCD_and_touch();
	void disp_create_and_touch_start();
	void init_chips();
	void init_buffers();
	void config_charts();
	void reconfig_charts();
	void h7();
	void sync();
	int sync_sender(const uint8_t& i);
	void all_H7_to_g4();
	void all_g4_to_H7();
	void update_cursor(const Chip& comp);
	void sender(const command& com, const uint8_t& adress, const uint8_t& compN, const dot& dot, const uint32_t& value);
	void UART4_send_address(const uint8_t& slave_address);
	void UART4_send_settings(const command& com, const uint8_t& compN, const uint8_t& dot, const uint32_t& value);
	void UART4_receive_settings();
	void UART4_send_settings_bootloader();
	void UART4_receive_settings_bootloader();
	int UART4_receive_timeout_10us();
	void DMA1_RX();
	void USART_noise_error_detected();
	void DMA_UART_ERRORS_HANDLER();
	int32_t convert_8_16(const uint8_t& a, const uint8_t& b);
	conv_16_8 convert_16_8(const uint32_t& a);
	void save_to_memory();
	void read_on_memory();
	void pause(const uint32_t& p);
	void debugg_fn(const std::string& str);  // DEBUG
	void debugg_clear();
	void resetPin();
	void send_test_midi();
	void DMA2_Stream3_i2c();
	void my_input_read(lv_indev_t* indev, lv_indev_data_t* data);
	void DMA2_Stream1_TransferComplete();
	void my_flush_cb(lv_display_t* disp, const lv_area_t* area, uint16_t* color_p);
	void Read_uint32(uint32_t Address, volatile uint32_t* pData, uint32_t Size);
	static inline void uint32_to_bytes_pointer(uint32_t value, uint8_t* bytes);
	static inline uint32_t bytes_to_uint32_pointer(const uint8_t* bytes);
	void Set_tx_s(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e);
	void data_from_H7_to_g4();
	void flash_g4(const uint32_t& addr, const int& chip_number);
	void jump_g4s_to_adress();
	void reset_bootloaders();
	void reset_main_to_bootloader();
	const comparator& searcher_addr_in_cursor(const uint32_t& chip);
	void set_cursor_piont_on(const comparator& comp);
	void set_cursor_piont_off(const comparator& comp);


	std::string debugg;
	std::string cursor_string;
	std::string chart_calib_online;
	std::string l;
	std::string r;
	std::string sensor_on_1_data_string;
	std::string sensor_on_2_data_string;
	std::string sensor_off_1_data_string;
	std::string sensor_off_2_data_string;

	lv_display_t* disp;
	lv_indev_t* indev;
	lv_obj_t* cur_shart;
	lv_chart_series_t* ser_on_green;
	lv_chart_series_t* ser_on_red;
	lv_chart_series_t* ser_on_blue;
	lv_chart_series_t* ser_on_dac;
	lv_chart_series_t* ser_off_green;
	lv_chart_series_t* ser_off_red;
	lv_chart_series_t* ser_off_blue;
	lv_chart_series_t* ser_off_dac;
	lv_style_t cursor_style;
	lv_chart_cursor_t* cursor_on_vert;
	lv_chart_cursor_t* cursor_on_hor;
	lv_chart_cursor_t* cursor_off_vert;
	lv_chart_cursor_t* cursor_off_hor;

	// настройки gpio для DISPLAY взяты отсюда: https://github.com/zeruns/STM32F407_LVGL_Template_MSP3526/blob/master/Core/Src/gpio.c
	const uint32_t BYTES_PER_PIXEL = (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB888)); // 565 or 888? (было 565)
	const uint32_t BUFF_SIZE = (480 * 20 * BYTES_PER_PIXEL);
	static lv_color16_t buf_1[BUFF_SIZE];
	static lv_color16_t buf_2[BUFF_SIZE];

	int32_t on_green_max = 4095;
	int32_t on_green_min = 0;
	int32_t on_red_max = 4095;
	int32_t on_red_min = 0;
	int32_t off_green_max = 4095;
	int32_t off_green_min = 0;
	int32_t off_red_max = 4095;
	int32_t off_red_min = 0;

}
#endif // extern "C"
