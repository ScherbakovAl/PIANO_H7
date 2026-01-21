/*
 * midi_keyboard_h7-2.cpp
 *
 *  Created on: Apr 8, 2025
 *      Author: sche
 */

#include <format>
#include "piano_h7.hpp"

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

#define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB888)) // TODO 565 or 888? (было 565)
#define BUFF_SIZE (480 * 20 * BYTES_PER_PIXEL)
static lv_color16_t buf_1[BUFF_SIZE]; // TODO 16 or 8
static lv_color16_t buf_2[BUFF_SIZE];

lv_display_t* disp;
lv_indev_t* indev;

lv_obj_t* cur_shart;
lv_chart_series_t* ser_on_green;
lv_chart_series_t* ser_on_red;
lv_chart_series_t* ser_on_blue;
lv_chart_cursor_t* c_on;
lv_chart_cursor_t* ch_on;
lv_style_t cursor_style;
lv_chart_series_t* ser_off_green;
lv_chart_series_t* ser_off_red;
lv_chart_series_t* ser_off_blue;
lv_chart_cursor_t* c_off;
lv_chart_cursor_t* ch_off;

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
//---------------------------------

// настройки gpio для DISPLAY взяты отсюда: https://github.com/zeruns/STM32F407_LVGL_Template_MSP3526/blob/master/Core/Src/gpio.c

void h7() {
	//	LL_mDelay(100);
	// TIM init
	LL_TIM_EnableCounter(TIM2); // просто счётчик (275Mhz)

	LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);
	LL_TIM_EnableAllOutputs(TIM1); // PWM - tim clk
	LL_TIM_EnableIT_TRIG(TIM1);

	LL_TIM_SetAutoReload(TIM3, allChipCount);
	LL_TIM_EnableCounter(TIM3); // считает номер контроллера g4

	LL_TIM_EnableCounter(TIM4); // для LVGL
	LL_TIM_EnableIT_UPDATE(TIM4);

	LL_TIM_EnableCounter(TIM5); // ограничение скорости сканирования плат

	//---------------------------------

	// UART init
	LL_USART_Enable(UART5);
	LL_USART_EnableDMAReq_RX(UART5);
	//---------------------------------

	// DMA RX для получения данных
	LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_2);
	LL_DMA_SetPeriphAddress(DMA1, LL_DMA_STREAM_2, (uint32_t) & (UART5->RDR));
	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_2, (uint32_t)rx_data);
	LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_2, dataLengthRX);
	LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_2); // включает прерывание transfer complete
	LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_2);
	//---------------------------------

	// LCD init
	LL_SPI_Enable(SPI3);
	LL_SPI_StartMasterTransfer(SPI3);
	LCD_Init();

	// TOUCH init
	// LL_I2C_Enable(I2C5);
	FT6336_Init();

	// LVGL init
	lv_init();

	// DISP start
	disp = lv_display_create(320, 480);
	lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB888);
	lv_display_set_flush_cb(disp, my_flush_cb);
	lv_display_set_buffers(disp, buf_1, buf_2, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

	// lv_display_set_flush_wait_cb(disp, my_flush_wait);

	// TOUCH start
	LL_TIM_EnableIT_UPDATE(TIM6); // для TOUCH
	indev = lv_indev_create();
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, my_input_read);

	// GUI start
	ui_init();

	// USB init
	tusb_init();
	//---------------------------------

	tud_task();
	lv_timer_handler();
	ui_tick();

	pause(10);
	send_test_midi();

	sync();
	initBuffers();
	configCharts();

// память
// SaveToMemory();
	ReadOnMemory(); // восстановление графика при включении
	debugg_fn("   -- -- Restore Calib DONE! -- --)"); // DEBUG
	LL_TIM_DisableCounter(TIM1); // PWM - tim clk
	LL_USART_DisableDMAReq_RX(UART5);
	all_H7_to_g4();
	LL_USART_EnableDMAReq_RX(UART5);
	LL_TIM_EnableCounter(TIM1); // PWM - tim clk
	debugg_fn("   -- H7 > >>>> > G4 DONE! --)"); // DEBUG
	//---------------------------------

	tud_task();
	lv_timer_handler();
	ui_tick();

// GPIOA->BSRR = 0x10; // for test // DEBUG
// GPIOA->BSRR = 0x100000;
// GPIOA->BSRR = 0x20; // for test // DEBUG
// GPIOA->BSRR = 0x200000;
// start PWM

	LL_TIM_EnableCounter(TIM1); // PWM - tim clk
	//---------------------------------

	pause(5); // DEBUG
	debugg_clear();
	debug_counter = 1;
	int test_int_timer2_old = test_int_timer2;

	while (1) {
		tud_task();
		// GPIOA->BSRR |= 0x10; // for test // DEBUG
		// lv_timer_handler();
		// GPIOA->BSRR |= 0x100000; // for test // DEBUG
		lv_timer_handler_run_in_period(10);
		ui_tick();

		if (TIM5->CNT > 3000) { // 1000 = 1ms (чтобы калибровка не наступала себе на пятки)
			if (cur_disp == on) {
				for (uint8_t adress = start_adress_chip_on; adress <= end_adress_chip_on; ++adress) {
					sender(command::all_calib, adress, 0, 0, subcommand::read_calibration);
					for (int i = 0; i < 7; ++i) {
						UART4_Receive_Settings();
						compsCHART_CALIB[(adress * 7) + i] = convert_8_16(a_, b_);
					}
					refresh_cursor(adress);
				}
				chart_calib_online = std::to_string(compsCHART_CALIB[cursor]);
				l = std::to_string(compsCHART_CALIB[cursor - 1]);
				r = std::to_string(compsCHART_CALIB[cursor + 1]);
				lv_chart_set_cursor_point(objects.chart_on, ch_on, ser_on_blue, cursor - 7);
				lv_chart_refresh(cur_shart);
			}

			if (cur_disp == off) {
				for (uint8_t adress = start_adress_chip_off; adress <= end_adress_chip_off; ++adress) {
					sender(command::all_calib, adress, 0, 0, subcommand::read_calibration);
					for (int i = 0; i < 7; ++i) {
						UART4_Receive_Settings();
						compsCHART_CALIB[(adress * 7) + i] = convert_8_16(a_, b_);
					}
					refresh_cursor(adress);
				}
				chart_calib_online = std::to_string(compsCHART_CALIB[cursor + 98]);
				l = std::to_string(compsCHART_CALIB[cursor + 98 - 1]);
				r = std::to_string(compsCHART_CALIB[cursor + 98 + 1]);
				lv_chart_set_cursor_point(objects.chart_off, ch_off, ser_off_blue, cursor);
				lv_chart_refresh(cur_shart);
			}
			TIM5->CNT = 0;
		}

			// if (test_memory < 196) { // for test
			// 	if (TIM2->CNT > 80000000) {
			// 		debugg_fn(std::format(" #{} CHART_0 = {}  CHART_1 = {}", test_memory, compsCHART_0[test_memory], compsCHART_1[test_memory]));
			// 		TIM2->CNT = 0;
			// 		++test_memory;
			// 	}
			// }

#define deb
#ifdef deb
		if (fl) {
			debugg_fn(std::format("tOut = {:.5f}", timerLenght_F));
			// test_t_out_fl = std::format("{:.10f}", timerLenght_F); // for test
			// 	// test_speed_fl = std::format("{:.3f}", speed_F);
			// debugg_fn(std::format("log = {:.3f}", speed_F));
			// 	// test_energy_fl = std::format("{:.3f}", energy_F);
				// debugg_fn(std::format("engy = {:.3f}", energy_F));
			// 	// test_midi_hi_fl = std::format("{:.3f}", midi_hi_F);
				// debugg_fn(std::format("mhi = {:.3f}", midi_hi_F));
			// 	// test_midi_lo_fl = std::format("{:.3f}", midi_lo_F);
				// debugg_fn(std::format("mlo = {:.3f}", midi_lo_F));
			// 	// test_timer2 = std::to_string(test_int_timer2);
			// 	// debugg_fn(std::to_string(test_int_timer2));
			// 	// mass_str = std::format("{:.7f} kgr", mass_to_disp);
			// 	// debugg_fn(std::format("{:.7f} kgr", mass_to_disp));
			// 	// note = std::format("{}, ship# {}", rx_data[0], rx_data[0] / 7);
			// debugg_fn(std::format(
			// 	"mhi {:.2f}, mlo {:.2f}",
			// 	midi_hi_F,
			// 	midi_lo_F
			// ));
			// debugg_fn(std::format(
			// 	"{}  ship {}     {}-{}-{}    {:.3f}.ms    {:.2f}.Hi   {:.2f}.Lo  {:.3f}.ms",
			// 	rx_data[0],
			// 	(rx_data[0] / 7),
			// 	rx_data[1],
			// 	rx_data[2],
			// 	rx_data[3],
			// 	timer_data_in,
			// 	midi_hi_F,
			// 	midi_lo_F,
			// 	(float)(test_int_timer2 - test_int_timer2_old) / 275000.0f
			// ));
			// test_int_timer2_old = test_int_timer2;
		// 	// t1 = std::to_string(rx_data[1]);
			// debugg_fn(std::format("1 = {}", rx_data[1]));
		// 	// t2 = std::to_string(rx_data[2]);
			// debugg_fn(std::format("2 = {}", rx_data[2]));
		// 	// t3 = std::to_string(rx_data[3]);
			// debugg_fn(std::format("3 = {}", rx_data[3]));
		// 	// timer_data = std::format("{:.4f}", timer_data_in);
			// debugg_fn(std::format("timer_data = {:.4f}", timer_data_in));
			fl = 0; // for test fl
		}
#endif
	}
} // h7

void sync() {
	LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
	LL_USART_DisableDMAReq_RX(UART5);
	TIM3->CNT = 0; // сбросить номер контроллера
	int fl_sync = 0; // for test
	for (uint8_t i = start_adress_chip_on; i <= end_adress_chip_on; ++i) {
		fl_sync += sync_sender(i);
	}
	for (uint8_t i = start_adress_chip_off; i <= end_adress_chip_off; ++i) {
		fl_sync += sync_sender(i);
	}
	if (fl_sync == 0) {
		debugg_fn("Sync DONE");
	}
	else {
		debugg_fn(std::format("Sync {} bugs", fl_sync));
	}
	LL_USART_EnableDMAReq_RX(UART5);
	LL_TIM_EnableCounter(TIM1);  // PWM - tim clk
}

int sync_sender(const uint8_t& i) {
	int fs = 0;
	UART4_SendAddress(i);
	pause(4);
	UART4_Send_Settings(command::sync_timer, 0, 0, 0);
	UART4_Receive_Settings();
	if (b_ != 0 && a_ != 0 && rx_settings[0] != i) {
		debugg_fn("Sync err, mcu  #" + std::to_string(i));
		++fs;
	}
	pause(1);
	return fs;
}

void check_max_min() {
	on_off_s1_s2_min_max mm; //  для сброса состояния max_min
	m_m = mm;
	if (cur_disp == on) {
		for (int i = 0; i < 98; ++i) {
			if (m_m.on.s_green.min > compsCHART_0[i]) {
				m_m.on.s_green.min = compsCHART_0[i];
			}
			if (m_m.on.s_green.max < compsCHART_0[i]) {
				m_m.on.s_green.max = compsCHART_0[i];
			}
			if (m_m.on.s_red.min > compsCHART_1[i]) {
				m_m.on.s_red.min = compsCHART_1[i];
			}
			if (m_m.on.s_red.max < compsCHART_1[i]) {
				m_m.on.s_red.max = compsCHART_1[i];
			}
		}

		s1_on_min = std::to_string(m_m.on.s_green.min);
		s1_on_max = std::to_string(m_m.on.s_green.max);
		s2_on_min = std::to_string(m_m.on.s_red.min);
		s2_on_max = std::to_string(m_m.on.s_red.max);
	}
	if (cur_disp == off) {
		for (int i = 98; i < 196; ++i) {
			if (m_m.off.s_green.min > compsCHART_0[i]) {
				m_m.off.s_green.min = compsCHART_0[i];
			}
			if (m_m.off.s_green.max < compsCHART_0[i]) {
				m_m.off.s_green.max = compsCHART_0[i];
			}
			if (m_m.off.s_red.min > compsCHART_1[i]) {
				m_m.off.s_red.min = compsCHART_1[i];
			}
			if (m_m.off.s_red.max < compsCHART_1[i]) {
				m_m.off.s_red.max = compsCHART_1[i];
			}
		}

		s1_off_min = std::to_string(m_m.off.s_green.min);
		s1_off_max = std::to_string(m_m.off.s_green.max);
		s2_off_min = std::to_string(m_m.off.s_red.min);
		s2_off_max = std::to_string(m_m.off.s_red.max);
	}
}

void all_H7_to_g4() {
	pause(10); // если вдруг кто-то захочет что-то отправить... ?
	for (uint8_t i = start_adress_chip_on * 7; i <= (end_adress_chip_on * 7) + 1; ++i) {
		sender(command::set_comp_value, i / 7, i % 7, 0, compsCHART_0[i]);
		sender(command::set_comp_value, i / 7, i % 7, 1, compsCHART_1[i]);
	}
	for (uint8_t i = start_adress_chip_off * 7; i <= (end_adress_chip_off * 7) + 6; ++i) {
		sender(command::set_comp_value, i / 7, i % 7, 0, compsCHART_0[i]);
		sender(command::set_comp_value, i / 7, i % 7, 1, compsCHART_1[i]);
		sender(command::set_comp_value, i / 7, i % 7, 2, compsCHART_0[i] - (compsCHART_0[i] / 10));
	}
}

void all_g4_to_H7() {
	pause(10); // если вдруг кто-то захочет что-то отправить... ?
	for (uint8_t i = start_adress_chip_on * 7; i <= (end_adress_chip_on * 7) + 1; ++i) {
		sender(command::read_comp_value, i / 7, i % 7, 0, 0);
		compsCHART_0[i] = convert_8_16(a_, b_);
		sender(command::read_comp_value, i / 7, i % 7, 1, 0);
		compsCHART_1[i] = convert_8_16(a_, b_);
	}
	for (uint8_t i = start_adress_chip_off * 7; i <= (end_adress_chip_off * 7) + 6; ++i) {
		sender(command::read_comp_value, i / 7, i % 7, 0, 0);
		compsCHART_0[i] = convert_8_16(a_, b_);
		sender(command::read_comp_value, i / 7, i % 7, 1, 0);
		compsCHART_1[i] = convert_8_16(a_, b_);
	}
}

void refresh_cursor(const uint8_t& adress) {
	for (int i = 0; i < 7; ++i) {
		const int c = (adress * 7) + i;
		bool fl_c = false;
		if (compsCHART_CALIB[c] < 4080 && compsCHART_CALIB[c] > 2) {

			if (compsCHART_CALIB_old[c] + 200 < compsCHART_CALIB[c]) {
				compsCHART_CALIB_old[c] = compsCHART_CALIB[c];
				fl_c = true;
			}
			else if (compsCHART_CALIB_old[c] - 200 > compsCHART_CALIB[c]) {
				compsCHART_CALIB_old[c] = compsCHART_CALIB[c];
				fl_c = true;
			}
		}
		if (fl_c) {
			if (c > 98) {
				cursor = c - 98;
				lv_chart_set_cursor_point(objects.chart_off, c_off, ser_off_green, cursor);
				sensor_off_1_data_string = std::to_string(compsCHART_0[c]);
				sensor_off_2_data_string = std::to_string(compsCHART_1[c]);
				cursor_string = std::to_string(cursor + 1);
			}
			else {
				cursor = c;
				lv_chart_set_cursor_point(objects.chart_on, c_on, ser_on_green, cursor - 7);
				sensor_on_1_data_string = std::to_string(compsCHART_0[c]);
				sensor_on_2_data_string = std::to_string(compsCHART_1[c]);
				cursor_string = std::to_string(cursor - 6);
			}
			fl_c = false;
		}
	}
}

void sender(const command& com, const uint8_t& adress, const uint8_t& compN, const uint8_t& dot,
	const uint32_t& value) {
	UART4_SendAddress(adress);
	pause(4); // 4 for release, 10-debug g4
	UART4_Send_Settings(com, compN, dot, value);
	UART4_Receive_Settings();
	pause(1); // 1 for release, 4-debug g4
	if (adress != rx_settings[0]) {
		debugg_fn(std::format("BAD ADRESS {}   rx_settings {}", adress, rx_settings[0]));
	}
	if (com == set_comp_value) {
		if (convert_8_16(a_, b_) != value) {
			debugg_fn(std::format("BAD SET DATA [0]= {}, add={}, comp={}, dot={}, value={}, in={}", rx_settings[0], adress, compN, dot, value, convert_8_16(a_, b_)));
		}
	}
}

// UART Send-Recive
void UART4_SendAddress(const uint8_t& slave_address) {
	const uint16_t address_byte = slave_address | 0x100; // Установка старшего бита (MSB) для указания адреса
	while (!LL_USART_IsActiveFlag_TXE(UART5)) {}
	LL_USART_TransmitData9(UART5, address_byte);
	while (!LL_USART_IsActiveFlag_TC(UART5)) {}
}

void UART4_Send_Settings(const command& com, const uint8_t& compN, const uint8_t& dot, const uint32_t& value) {
	tx_settings[0] = { (uint8_t)com };
	tx_settings[1] = { compN };
	tx_settings[2] = { dot };
	conv_16_8 c;
	c = convert_16_8(value);
	tx_settings[3] = c.a;
	tx_settings[4] = c.b;
	while (!LL_USART_IsActiveFlag_TXE(UART5)) {}
	for (uint16_t i = 0; i < tx_settings_length; i++) {
		LL_USART_TransmitData9(UART5, tx_settings[i]);
		while (!LL_USART_IsActiveFlag_TXE(UART5)) {}
	}
	while (!LL_USART_IsActiveFlag_TC(UART5)) {}
}

void UART4_Receive_Settings() {
	for (uint8_t i = 0; i < rx_settings_length; i++) {
		while (!LL_USART_IsActiveFlag_RXNE(UART5)) {}
		rx_settings[i] = (uint8_t)LL_USART_ReceiveData9(UART5);
	}
	compN_ = rx_settings[1];
	dot_ = rx_settings[2];
	a_ = rx_settings[3];
	b_ = rx_settings[4];
}
//---------------------------------

const float key_mass = 0.008f; // 8 гр -->> переехал в массив
const float distance_F = 0.0017f; // 1.7 мм (толщина шаблонов 1.9 и 0.2)
const float div_on = 0.000000000092f; // меньше - громче
const float div_off = 0.00000000004f; // меньше - громче 
const float deriv_F = 2.0f; // делить на 2 в формуле
const float maxMidi_F = 127.99f;

// DMA IQR Handler
void DMA1_RX(void) {

	LL_DMA_ClearFlag_TC2(DMA1);
	LL_TIM_DisableCounter(TIM1); // PWM - tim clk

	if (LL_USART_IsActiveFlag_NE(UART5)) { // DEBUG // поиск ошибок связи
		LL_USART_ClearFlag_NE(UART5);
		USART_Noise_Error_detected();
		debugg_fn("...noise");
	}
	else {

		SCB_CleanInvalidateDCache(); // or
		// SCB_CleanInvalidateDCache_by_Addr((uint32_t*)(((uint32_t)rx_data) & ~(uint32_t)0x1F), dataLengthRX);

		const int rxB = rx_data[0];

		midi_hi_F = 0; // DEBUG
		midi_lo_F = 0; // DEBUG
		timer_data_in = 0; // DEBUG


		if (rxB > 168) { // DEBUG
			USART_Noise_Error_detected(); // DEBUG
			debugg_fn("... rxB > 168");
		}

		// if (rx_data[1] > 3) { // DEBUG
		// 	USART_Noise_Error_detected(); // DEBUG
		// 	debugg_fn("... t > 3");
		// }
		uint32_t tOut = 0;
		float integerPart_F;
		tOut = rx_data[1] << 16 | rx_data[2] << 8 | rx_data[3];
		int note_ = rxB + noteAdder[rxB];

		// #define speee
#ifdef speee

		if (rxB < 98) {
			timerLenght_F = (float)tOut * div_on;
		}
		else {
			timerLenght_F = (float)tOut * div_off;
		}

		speed_F = distance_F / timerLenght_F;
		energy_F = (mass_F[rxB] * speed_F * speed_F) / deriv_F;
		midi_hi_F = energy_F / maxMidi_F;
		midi_lo_F = modf(midi_hi_F, &integerPart_F) * maxMidi_F;

		if (midi_hi_F < 1) {
			midi_hi_F = 1;
			midi_lo_F = 1;
		}

		if (midi_hi_F > 127) {
			midi_hi_F = 127;
			midi_lo_F = 127;
		}

		uint8_t note_buf[] = {
			0xB0,
			0x58,
			(uint8_t)midi_lo_F,
			rxB < 98 ? 0x90 : 0x80, // 0x90 note on
			note_,
			(uint8_t)midi_hi_F
		};

		tud_midi_stream_write(0, note_buf, 6);

#endif

#ifndef speee
		//47.9 + 51.23(17000)-2474.3
		midi_hi_F = 47.9 + (51.23 * log10f(17000.0 / ((float)tOut-3800.3))); // 74 + 78? // 57.96 + 100? // 57.96 + 71.3? // 70 + 74(17000)?
		midi_lo_F = modf(midi_hi_F, &integerPart_F) * maxMidi_F;
		note_ = rxB + noteAdder[rxB];

		if (midi_hi_F < 1) {
			midi_hi_F = 1;
			midi_lo_F = 1;
		}

		if (midi_hi_F > 127) {
			midi_hi_F = 127;
			midi_lo_F = 127;
		}

		uint8_t note_buf2[] = {
			0xB0,
			0x58,
			(uint8_t)midi_lo_F,
			rxB < 98 ? 0x90 : 0x80, // 0x90 note on
			note_,
			(uint8_t)midi_hi_F
		};

		tud_midi_stream_write(0, note_buf2, 6);

		fl = rxB < 98 ? 1 : 0; // for test // разрешить обновлять цифры на дисплее
		timerLenght_F = midi_hi_F; // DEBUG
		// speed_F = midi_hi_F; // DEBUG
#endif
	}
	LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_2);
	LL_TIM_EnableCounter(TIM1); // PWM - tim clk // for test // TODO правильно ли здесь это использовать?

}

void USART_Noise_Error_detected() {
	debugg_fn(std::format("USART Noise Error detected {}-{}-{}-{}", rx_data[0], rx_data[1], rx_data[2], rx_data[3]));
	LL_USART_RequestRxDataFlush(UART5); // TODO // for test // ????
	SCB_CleanInvalidateDCache();
}

void DMA_UART_ERRORS_HANDLER() {
	// if (LL_USART_IsActiveFlag_NE(UART5)) {
		// debugg_fn("USART Noise error detected ");
		// LL_USART_DisableDMAReq_RX(UART5); // DEBUG оно здесь помогает очистить от ошибок?
		// LL_USART_ClearFlag_NE(UART5);
		// LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_2);
		// LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_2, dataLengthRX);
		// LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_2);
		// LL_DMA_SetPeriphAddress(DMA1, LL_DMA_STREAM_2, (uint32_t) & (UART5->RDR));
		// LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_2, (uint32_t)rx_data);
		// LL_DMA_ClearFlag_HT2(DMA1);
		// LL_DMA_ClearFlag_TC2(DMA1);
		// LL_DMA_ClearFlag_TE2(DMA1);
		// LL_DMA_ClearFlag_DME2(DMA1);
		// LL_USART_RequestRxDataFlush(UART5);
		// LL_USART_EnableDMAReq_RX(UART5); // ?
	// }
}

// tim_IN = 100ns на значение
// 35445 = 22.211 midi

// v = s / t
// s - расстояние
// s = 2 mm = 0.002 m
// t - время
// t = 62000 ns = 0.000062 s

// v = 0.002 / 0.000062 = 32,258064516 м/с;
// v = 2000 / 62 = 32,258064516; ~~~
// v = 2000000 / 62000 = 32,258064516;

// A - кинетическая энергия
// A = M * v * v / 2;

// М - масса (кг)
// M = 8 g = 0.008 kg
// v * v - скорость в квадрате (м/с)

// A = 0.02 * 32,258064516 * 32,258064516 / 2 = 10,405827263;
//---------------------------------

void initBuffers() {
	for (int i = 0; i < 196; ++i) {
		if (i < 98) {
			compsCHART_0[i] = def_on[0];
			compsCHART_1[i] = def_on[1];
		}
		else {
			compsCHART_0[i] = def_off[0];
			compsCHART_1[i] = def_off[1];
		}
		compsCHART_CALIB[i] = 800;
	}

	for (uint i = 1; i < 196; ++i) { // note shift // TODO проверить здесь что происходит...
		if (i < 55) {
			noteAdder[i] = 14;
		}
		if (i > 54 && i < 96) {
			noteAdder[i] = 13;
		}
		if (i > 95 && i < 146) {
			noteAdder[i] = -77;
		}
		if (i > 145 && i < 200) {
			noteAdder[i] = -78;
		}
	}

	for (uint i = 0; i < 196; ++i) {
		mass_F[i] = key_mass; //  + (float)i / 10000000; // 8 гр
	}
}

int32_t convert_8_16(const uint8_t& a, const uint8_t& b) {
	int32_t x = a << 8 | b;
	return x;
}

conv_16_8 convert_16_8(const uint32_t& a) {
	conv_16_8 r;
	// TODO AI - Проблема: Неправильный порядок операций. Должно быть r.a = (a & (0xff << 8)) >> 8; или r.a = (a >> 8) & 0xff;
	r.a = (a & 0xff << 8) >> 8;
	r.b = a & 0xff;
	return r;
}

void chart_correction(const uint32_t& x, const plus_minus& pm) {
	if (lv_scr_act() == objects.d_chart_manual_edit_on) {
		if (col_but == green)
			pm == plus ? compsCHART_0[cursor] += x : compsCHART_0[cursor] -= x;
		if (col_but == red)
			pm == plus ? compsCHART_1[cursor] += x : compsCHART_1[cursor] -= x;
	}
	else if (lv_scr_act() == objects.d_chart_manual_edit_off) {
		if (col_but == green)
			pm == plus ? compsCHART_0[cursor] += x : compsCHART_0[cursor] -= x;
		if (col_but == red)
			pm == plus ? compsCHART_1[cursor] += x : compsCHART_1[cursor] -= x;
	}
	else if (lv_scr_act() == objects.d_chart_graph_resize_off) {
		if (col_but == red) {
			if (top_bot == bot) {
				pm == minus ? off_red_min += x : off_red_min -= x;
			}
			if (top_bot == top) {
				pm == minus ? off_red_max += x : off_red_max -= x;
			}
		}
		if (col_but == green) {
			if (top_bot == bot) {
				pm == minus ? off_green_min += x : off_green_min -= x;
			}
			if (top_bot == top) {
				pm == minus ? off_green_max += x : off_green_max -= x;
			}
		}
		lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_PRIMARY_Y, (int32_t)off_green_min, (int32_t)off_green_max);
		lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_SECONDARY_Y, (int32_t)off_red_min, (int32_t)off_red_max);
	}
	else if (lv_scr_act() == objects.d_chart_graph_resize_on) {
		if (col_but == red) {
			if (top_bot == bot) {
				pm == minus ? on_red_min += x : on_red_min -= x;
			}
			if (top_bot == top) {
				pm == minus ? on_red_max += x : on_red_max -= x;
			}
		}
		if (col_but == green) {
			if (top_bot == bot) {
				pm == minus ? on_green_min += x : on_green_min -= x;
			}
			if (top_bot == top) {
				pm == minus ? on_green_max += x : on_green_max -= x;
			}
		}
		lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_PRIMARY_Y, (int32_t)on_green_min, (int32_t)on_green_max);
		lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_SECONDARY_Y, (int32_t)on_red_min, (int32_t)on_red_max);
	}
	check_max_min();
	lv_chart_refresh(cur_shart);
}

void SaveToMemory() {

	SCB_DisableICache();
	SCB_DisableDCache();
	HAL_FLASH_Unlock();

	FLASH_Erase_Sector(FLASH_SECTOR_7, FLASH_BANK_1, FLASH_VOLTAGE_RANGE_2);

	uint32_t Addr = Flash_Address;
	for (uint32_t i = 0; i < sizeCHART_BUFFER; i += 8) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, Addr, (uint32_t)&compsCHART_0[i]) != HAL_OK) {
			HAL_FLASH_Lock();
			return;
		}
		Addr += 0x20;
	}
	for (uint32_t i = 0; i < sizeCHART_BUFFER; i += 8) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, Addr, (uint32_t)&compsCHART_1[i]) != HAL_OK) {
			HAL_FLASH_Lock();
			return;
		}
		Addr += 0x20;
	}

	// // замок на запись (по адресу Flash_Address + 0x640)
	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, Addr, (uint32_t)&key_to_change_memory) != HAL_OK) {
		HAL_FLASH_Lock();
		return;
	}

	HAL_FLASH_Lock();
	SCB_EnableICache();
	SCB_EnableDCache();
}

void ReadOnMemory() {
	// if ((*(volatile uint32_t*)(Flash_Address + 0x640)) != key_to_change_memory) {
	// 	SaveToMemory();
	// }
	// else {
	for (int i = 0; i < sizeCHART_BUFFER; ++i) {
		compsCHART_0[i] = *(volatile uint32_t*)(Flash_Address + (i * sizeof(uint32_t)));
		compsCHART_1[i] = *(volatile uint32_t*)(Flash_Address + (i * sizeof(uint32_t)) + 0x320);
	}
// }
}

// (1uS)
void pause(const uint32_t& p) {
	TIM2->CNT = 0;
	uint32_t t = p * 275;
	while (TIM2->CNT < t) {
	}
}

void debugg_fn(const std::string& str) {  // DEBUG
	if (debug_counter % 10 == 0)debugg_clear();
	if (debug_counter) debugg += "\n";
	debugg += std::to_string(debug_counter);
	debugg += "        ";
	debugg += str;
	++debug_counter;
}

void debugg_clear() {
	debugg.clear();
}

void resetPin() {
	NVIC_SystemReset();
}

void send_test_midi() { // for test   // TODO можно удалить
	uint8_t const cable_num = 0;
	uint8_t note_buf[] = { 0xB0, 0x58, 16, 0x90, 64, 0x36, 0xB0, 0x58, 125, 0x80, 64, 0x36 };
	const int bufsize = sizeof(note_buf);
	tud_midi_stream_write(cable_num, note_buf, bufsize);
	pause(100);
	tud_midi_stream_write(cable_num, note_buf, bufsize);
}

// LVGL UTILITES
//---------------------------------

void DMA2_Stream3_i2c(void) { // DMA touch - панели
	// Обработка Transfer Complete
	if (LL_DMA_IsActiveFlag_TC3(DMA2)) {
		LL_DMA_ClearFlag_TC3(DMA2);
		ft6336_dma_rx_complete = 1;
	}

	// Обработка Transfer Error
	// if (LL_DMA_IsActiveFlag_TE3(DMA2)) {
	// 	LL_DMA_ClearFlag_TE3(DMA2);
	// 	ft6336_dma_error = 1;
	// }

	// Обработка Half Transfer (опционально)
	// if (LL_DMA_IsActiveFlag_HT3(DMA2)) {
	// 	LL_DMA_ClearFlag_HT3(DMA2);
	// }

	// Обработка Direct Mode Error (опционально)
	// if (LL_DMA_IsActiveFlag_DME3(DMA2)) {
	// 	LL_DMA_ClearFlag_DME3(DMA2);
	// }
}

void my_input_read(lv_indev_t* indev, lv_indev_data_t* data) {
	// if (touchpad_pressed) {
	// 	TouchPoints_HandleTypeDef TP = FT6336_GetTouchPoint();
	// 	data->point.x = TP.point1_x;
	// 	data->point.y = TP.point1_y;
	// 	data->state = LV_INDEV_STATE_PRESSED;
	// }
	// else {
	// 	data->state = LV_INDEV_STATE_RELEASED;
	// }

	uint8_t touchStatus = 0;
	FT6336_ReadRegister(FT6336_TD_STATUS, &touchStatus, 1);  // читаем 0x02
	uint8_t touchCount = touchStatus & 0x0F;
	if (touchCount > 0) {
		TouchPoints_HandleTypeDef TP = FT6336_GetTouchPoint();
		data->point.x = TP.point1_x;
		data->point.y = TP.point1_y;
		data->state = LV_INDEV_STATE_PRESSED;
	}
	else {
		data->state = LV_INDEV_STATE_RELEASED;
	}
}

void DMA2_Stream1_TransferComplete() { // DMA дисплея
	// Проверка флага Transfer Complete
	if (LL_DMA_IsActiveFlag_TC1(DMA2)) {
		LL_DMA_ClearFlag_TC1(DMA2);
	}

	// Проверка флага Transfer Error
	if (LL_DMA_IsActiveFlag_TE1(DMA2)) {
		LL_DMA_ClearFlag_TE1(DMA2);
		// Обработка ошибки
	}

	// Проверка флага Half Transfer (если нужно)
	if (LL_DMA_IsActiveFlag_HT1(DMA2)) {
		LL_DMA_ClearFlag_HT1(DMA2);
	}

	lv_display_flush_ready(disp);
}

// typedef void (*lv_display_flush_cb_t)(lv_display_t * disp, const lv_area_t * area, uint16_t * px_map); >>>  lv_display.h ( uint16_t !!! ) !!
void my_flush_cb(lv_display_t* disp, const lv_area_t* area, uint16_t* color_p) {
	LCD_SetWindows(area->x1, area->y1, area->x2, area->y2);
	const int32_t height = area->y2 - area->y1 + 1;
	const int32_t width = area->x2 - area->x1 + 1;
	const int32_t wh_ = width * height * 3;

	// for (int32_t i = 0; i < width * height; i++) {
		// 	LCD_Send_Data_16(color_p);
		// 	++color_p;
		// }
		// lv_display_flush_ready(disp);

	// SCB_CleanInvalidateDCache_by_Addr((uint32_t*)(((uint32_t)color_p) & ~(uint32_t)0x1F), wh_ + 32);
	SCB_CleanInvalidateDCache(); // or
	Send_DMA_Data8(color_p, wh_);
}

void my_flush_wait(lv_display_t* disp) {
	lv_display_flush_ready(disp);
}

// LVGL ACTIONS
#ifdef __cplusplus
extern "C" {
	void configCharts() {
		lv_obj_t* ob = objects.chart_on;
		lv_chart_set_point_count(ob, 89);
		ser_on_blue = lv_chart_add_series(ob, lv_color_hex(0x314ded), LV_CHART_AXIS_PRIMARY_X); // LV_COLOR_MAKE(0xE9, 0x1E, 0x63)
		ser_on_green = lv_chart_add_series(ob, lv_color_hex(0x0aaa37), LV_CHART_AXIS_PRIMARY_X);
		ser_on_red = lv_chart_add_series(ob, lv_color_hex(0xdb591e), LV_CHART_AXIS_PRIMARY_X);
		lv_chart_set_series_ext_y_array(ob, ser_on_green, &compsCHART_0[7]);
		lv_chart_set_series_ext_y_array(ob, ser_on_red, &compsCHART_1[7]);
		lv_chart_set_series_ext_y_array(ob, ser_on_blue, &compsCHART_CALIB[7]);
		lv_chart_set_axis_range(ob, LV_CHART_AXIS_PRIMARY_Y, on_green_max, on_green_min);
		lv_chart_set_axis_range(ob, LV_CHART_AXIS_SECONDARY_Y, on_red_max, on_red_min);
		c_on = lv_chart_add_cursor(ob, lv_color_hex(0x808080), LV_DIR_VER);
		ch_on = lv_chart_add_cursor(ob, lv_color_hex(0x808080), LV_DIR_HOR);
		lv_chart_set_cursor_point(ob, c_on, ser_on_green, cursor);
		lv_chart_set_cursor_point(ob, ch_on, ser_on_blue, cursor);
		lv_obj_set_style_line_width(ob, 1, LV_PART_CURSOR); // толщина курсора
		lv_obj_set_style_line_width(ob, 0, LV_PART_ITEMS);  // толщина линий между точками на графике
		lv_obj_set_style_size(ob, 2, 3, LV_PART_INDICATOR); // размер точек на графике
		lv_chart_set_div_line_count(ob, 0, 0);
		lv_obj_set_style_radius(ob, 0, 0);

		ob = objects.chart_off;
		lv_chart_set_point_count(ob, 70);
		ser_off_blue = lv_chart_add_series(ob, lv_color_hex(0x314ded), LV_CHART_AXIS_PRIMARY_X);
		ser_off_green = lv_chart_add_series(ob, lv_color_hex(0x0aaa37), LV_CHART_AXIS_PRIMARY_X);
		ser_off_red = lv_chart_add_series(ob, lv_color_hex(0xdb591e), LV_CHART_AXIS_PRIMARY_X);
		lv_chart_set_series_ext_y_array(ob, ser_off_green, &compsCHART_0[98]);
		lv_chart_set_series_ext_y_array(ob, ser_off_red, &compsCHART_1[98]);
		lv_chart_set_series_ext_y_array(ob, ser_off_blue, &compsCHART_CALIB[98]);
		lv_chart_set_axis_range(ob, LV_CHART_AXIS_PRIMARY_Y, off_green_max, off_green_min);
		lv_chart_set_axis_range(ob, LV_CHART_AXIS_SECONDARY_Y, off_red_max, off_red_min);
		c_off = lv_chart_add_cursor(ob, lv_color_hex(0x808080), LV_DIR_VER); // lv_color_make(200, 200, 200)
		ch_off = lv_chart_add_cursor(ob, lv_color_hex(0x808080), LV_DIR_HOR);
		lv_chart_set_cursor_point(ob, c_off, ser_off_green, cursor);
		lv_chart_set_cursor_point(ob, ch_off, ser_off_blue, cursor);
		lv_obj_set_style_line_width(ob, 1, LV_PART_CURSOR); // толщина курсора
		lv_obj_set_style_line_width(ob, 0, LV_PART_ITEMS);  // толщина линий между точками на графике
		lv_obj_set_style_size(ob, 2, 3, LV_PART_INDICATOR); // размер точек на графике
		lv_chart_set_div_line_count(ob, 0, 0);
		lv_obj_set_style_radius(ob, 0, 0);
	}

	void action_to_main_disp(lv_event_t* e) {
		pause(100);
		if (cur_disp == current_display::on) {
			for (uint8_t adress = start_adress_chip_on; adress <= end_adress_chip_on; ++adress) {
				sender(command::all_calib, adress, 0, 0, ::stop_calibration);
			}
		}
		if (cur_disp == current_display::off) {
			for (uint8_t adress = start_adress_chip_off; adress <= end_adress_chip_off; ++adress) {
				sender(command::all_calib, adress, 0, 0, ::stop_calibration);
			}
		}
		cur_disp = dis_main;
		loadScreen(SCREEN_ID_D_MAIN);
		debugg_clear();
		sync();
		LL_USART_EnableDMAReq_RX(UART5);
		LL_TIM_EnableCounter(TIM1);  // PWM - tim clk
	}

	void action_to_disp_calibration_on(lv_event_t* e) {
		LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
		LL_USART_DisableDMAReq_RX(UART5);
		cur_disp = on;
		cur_shart = objects.chart_on;
		// lv_obj_set_parent(objects.chart_on, objects.d_chart_calib_on);
		loadScreen(SCREEN_ID_D_CHART_CALIB_ON);
		debugg_clear();
		all_g4_to_H7();
		for (uint8_t adress = start_adress_chip_on; adress <= end_adress_chip_on; ++adress) { // TODO
			sender(command::all_calib, adress, 0, 0, subcommand::start_calibration);
		}
	}

	void action_to_disp_calibration_off(lv_event_t* e) {
		LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
		LL_USART_DisableDMAReq_RX(UART5);
		cur_disp = off;
		cur_shart = objects.chart_off;
		// lv_obj_set_parent(objects.chart_off, objects.d_chart_calib_off);
		loadScreen(SCREEN_ID_D_CHART_CALIB_OFF);
		debugg_clear();
		all_g4_to_H7();
		for (uint8_t adress = start_adress_chip_off; adress <= end_adress_chip_off; ++adress) { // TODO
			sender(command::all_calib, adress, 0, 0, subcommand::start_calibration);
		}
	}

	void action_to_disp_manual_edit_on(lv_event_t* e) {
		debugg_clear();
		cur_disp = on;
		cur_shart = objects.chart_on;
		check_max_min();
		action_s1__s2_upd(e);
		lv_obj_set_parent(objects.chart_on, objects.d_chart_manual_edit_on);
		loadScreen(SCREEN_ID_D_CHART_MANUAL_EDIT_ON);
	}

	void action_to_disp_manual_edit_off(lv_event_t* e) {
		debugg_clear();
		cur_disp = off;
		cur_shart = objects.chart_off;
		check_max_min();
		action_s1__s2_upd(e);
		lv_obj_set_parent(objects.chart_off, objects.d_chart_manual_edit_off);
		loadScreen(SCREEN_ID_D_CHART_MANUAL_EDIT_OFF);
	}

	void action_to_disp_graph_resize_on(lv_event_t* e) {
		debugg_clear();
		cur_disp = on;
		cur_shart = objects.chart_on;
		check_max_min();
		action_s1__s2_upd(e);
		lv_obj_set_parent(objects.chart_on, objects.d_chart_graph_resize_on);
		loadScreen(SCREEN_ID_D_CHART_GRAPH_RESIZE_ON); // TODO loadScreen поставить в самый верх?
	}

	void action_to_disp_graph_resize_off(lv_event_t* e) {
		debugg_clear();
		cur_disp = off;
		cur_shart = objects.chart_off;
		check_max_min();
		action_s1__s2_upd(e);
		lv_obj_set_parent(objects.chart_off, objects.d_chart_graph_resize_off);
		loadScreen(SCREEN_ID_D_CHART_GRAPH_RESIZE_OFF);
	}

	void action_to_disp_back(lv_event_t* e) {
		debugg_clear();
		if (cur_disp == on) {
			cur_shart = objects.d_chart_calib_on;
			lv_obj_set_parent(objects.chart_on, objects.d_chart_calib_on);
			loadScreen(SCREEN_ID_D_CHART_CALIB_ON);
		}
		else {
			cur_disp = off;
			cur_shart = objects.d_chart_calib_off;
			lv_obj_set_parent(objects.chart_off, objects.d_chart_calib_off);
			loadScreen(SCREEN_ID_D_CHART_CALIB_OFF);
		}
	}

	void action_calib_sensor_1_on(lv_event_t* e) {
		const uint8_t adr = cursor / 7;
		const uint8_t c = cursor % 7;
		const uint8_t d = 0;
		sender(command::set_comp_value, adr, c, d, compsCHART_CALIB[cursor]);
		compsCHART_0[cursor] = convert_8_16(a_, b_);
		sensor_on_1_data_string = std::to_string(compsCHART_0[cursor]);
		lv_chart_refresh(cur_shart);
	}

	void action_calib_sensor_2_on(lv_event_t* e) {
		const uint8_t adr = cursor / 7;
		const uint8_t c = cursor % 7;
		const uint8_t d = 1;
		sender(command::set_comp_value, adr, c, d, compsCHART_CALIB[cursor]);
		compsCHART_1[cursor] = convert_8_16(a_, b_);
		sensor_on_2_data_string = std::to_string(compsCHART_1[cursor]);
		lv_chart_refresh(cur_shart);
	}

	void action_calib_sensor_1_off(lv_event_t* e) {
		const uint8_t cu = cursor + 98;
		const uint8_t adr = cu / 7;
		const uint8_t c = cu % 7;
		const uint8_t d = 0;
		sender(command::set_comp_value, adr, c, d, compsCHART_CALIB[cu]);
		compsCHART_0[cu] = convert_8_16(a_, b_);
		sensor_off_1_data_string = std::to_string(compsCHART_0[cu]);
		lv_chart_refresh(cur_shart);
	}

	void action_calib_sensor_2_off(lv_event_t* e) {
		const uint8_t cu = cursor + 98;
		const uint8_t adr = cu / 7;
		const uint8_t c = cu % 7;
		const uint8_t d = 1;
		sender(command::set_comp_value, adr, c, d, compsCHART_CALIB[cu]);
		compsCHART_1[cu] = convert_8_16(a_, b_);
		sensor_off_2_data_string = std::to_string(compsCHART_1[cu]);
		lv_chart_refresh(cur_shart);
	}


	void set_cursor_piont_on() {
		cursor_string = std::to_string(cursor - 6);
		lv_chart_set_cursor_point(objects.chart_on, c_on, ser_on_green, cursor - 7);
		sensor_on_1_data_string = std::to_string(compsCHART_0[cursor]);
		sensor_on_2_data_string = std::to_string(compsCHART_1[cursor]);
	}

	void set_cursor_piont_off() {
		cursor_string = std::to_string(cursor + 1);
		lv_chart_set_cursor_point(objects.chart_off, c_off, ser_off_green, cursor);
		sensor_off_1_data_string = std::to_string(compsCHART_0[cursor + 98]);
		sensor_off_2_data_string = std::to_string(compsCHART_1[cursor + 98]);
	}

	void action_cursor_minus(lv_event_t* e) {
		if (cur_disp == on) {
			if (cursor > 7) {
				cursor = cursor - 1;
			}
			set_cursor_piont_on();
		}
		else {
			if (cursor > 0) {
				cursor = cursor - 1;
			}
			set_cursor_piont_off();
		}
	}

	void action_cursor_plus(lv_event_t* e) {
		if (cur_disp == on) {
			if (cursor < 95) {
				cursor = cursor + 1;
			}
			set_cursor_piont_on();
		}
		else {
			if (cursor < 69) {
				cursor = cursor + 1;
			}
			set_cursor_piont_off();
		}
	}

	void action_cursor_minus_7(lv_event_t* e) {
		if (cur_disp == on) {
			if (cursor > 13) {
				cursor = cursor - 7;
			}
			set_cursor_piont_on();
		}
		else {
			if (cursor > 6) {
				cursor = cursor - 7;
			}
			set_cursor_piont_off();
		}
	}

	void action_cursor_plus_7(lv_event_t* e) {
		if (cur_disp == on) {
			if (cursor < 89) {
				cursor = cursor + 7;
			}
			set_cursor_piont_on();
		}
		else {
			if (cursor < 63) {
				cursor = cursor + 7;
			}
			set_cursor_piont_off();
		}
	}

	void action_save_calibration(lv_event_t* e) {
		SaveToMemory();
		debugg_fn("Save calib"); // DEBUG
	}

	void action_restore_calibration(lv_event_t* e) {
		ReadOnMemory();
		debugg_fn("Restore calib"); // DEBUG
	}

	void action_to_disp_divisible_edit(lv_event_t* e) {
		loadScreen(SCREEN_ID_DIVISIBLE_EDIT_DISP);
	}

	void action_s1__s2_upd(lv_event_t* e) {
		col_but = c_none;
		if (lv_scr_act() == objects.d_chart_manual_edit_on) {

			if (lv_obj_get_state(objects.s1_s2_on) == 16) { // == зелёная
				ch_o = "green";
				col_but = green;
			}
			else if (lv_obj_get_state(objects.s1_s2_on) == 17) { // == красная
				ch_o = "red";
				col_but = red;
			}
			else {
				ch_o = "press";
				col_but = c_none;
			}
		}
		else if (lv_scr_act() == objects.d_chart_manual_edit_off) {

			if (lv_obj_get_state(objects.s1_s2_off) == 16) { // == зелёная
				ch_f = "green";
				col_but = green;
			}
			else if (lv_obj_get_state(objects.s1_s2_off) == 17) { // == красная
				ch_f = "red";
				col_but = red;
			}
			else {
				ch_f = "press";
				col_but = c_none;
			}
		}
		else if (lv_scr_act() == objects.d_chart_graph_resize_on) {

			if (lv_obj_get_state(objects.s1_s2_button) == 16) { // == зелёная
				disp_on_off_button = "green";
				col_but = green;
			}
			else if (lv_obj_get_state(objects.s1_s2_button) == 17) { // == красная
				disp_on_off_button = "red";
				col_but = red;
			}
			else {
				disp_on_off_button = "press";
				col_but = c_none;
			}
		}
		else if (lv_scr_act() == objects.d_chart_graph_resize_off) {

			if (lv_obj_get_state(objects.s1_s2_button_3) == 16) { // == зелёная
				disp_on_off_button_3 = "green";
				col_but = green;
			}
			else if (lv_obj_get_state(objects.s1_s2_button_3) == 17) { // == красная
				disp_on_off_button_3 = "red";
				col_but = red;
			}
			else {
				disp_on_off_button_3 = "press";
				col_but = c_none;
			}
		}
	}

	void action_top_bot(lv_event_t* e) {
		top_bot = t_none;
		if (lv_scr_act() == objects.d_chart_graph_resize_on) {

			if (lv_obj_get_state(objects.s1_s2_button_1) == 16) { // == зелёная
				top_bot_str = "top";
				top_bot = top;
			}
			else if (lv_obj_get_state(objects.s1_s2_button_1) == 17) { // == красная
				top_bot_str = "bottom";
				top_bot = bot;
			}
			else {
				top_bot_str = "press";
				top_bot = t_none;
			}
		}
		else if (lv_scr_act() == objects.d_chart_graph_resize_off) {

			if (lv_obj_get_state(objects.s1_s2_button_2) == 16) { // == зелёная
				top_bot_str_2 = "top";
				top_bot = top;
			}
			else if (lv_obj_get_state(objects.s1_s2_button_2) == 17) { // == красная
				top_bot_str_2 = "bottom";
				top_bot = bot;
			}
			else {
				top_bot_str_2 = "press";
				top_bot = t_none;
			}
		}
	}
	extern uint32_t* dfu_boot_flag;
	void action_piano_off(lv_event_t* e) {
		SCB_DisableDCache();
		SCB_DisableICache();
		*dfu_boot_flag = 0xDEADBEEF;
		pause(200000);
		NVIC_SystemReset();
	}

	void action_pre_pressure_switching(lv_event_t* e) {  // TODO можно удалить (кнопка)
		// TODO: pre-pres switching
	}

	void action_set(lv_event_t* e) {  // TODO можно удалить (кнопка)
		// const uint8_t adr = cursor / 7;
		// const uint8_t c = cursor % 7;
		// if (lv_scr_act() == objects.d_chart_calib_on) {
		// 	setCompValue(adr, c, 0U, compsCHART_0[cursor]); // for green
		// 	setCompValue(adr, c, 1U, compsCHART_1[cursor]); // for red
		// }
		// else if (lv_scr_act() == objects.d_chart_manual_edit_on) {
		// 	if (col_but == green) {
		// 		setCompValue(adr, c, 0, compsCHART_0[cursor]); // for green
		// 	}
		// 	else {
		// 		setCompValue(adr, c, 1, compsCHART_1[cursor]); // for red
		// 	}
		// }
		// else if (lv_scr_act() == objects.d_chart_calib_off) {
		// 	setCompValue(adr + 14, c, 0, compsCHART_0[cursor]); // for green
		// 	setCompValue(adr + 14, c, 1, compsCHART_1[cursor]); // for red
		// }
		// else if (lv_scr_act() == objects.d_chart_manual_edit_off) {
		// 	if (col_but == green) {
		// 		setCompValue(adr + 14, c, 0, compsCHART_0[cursor]); // for green
		// 	}
		// 	else {
		// 		setCompValue(adr + 14, c, 1, compsCHART_1[cursor]); // for red
		// 	}
		// }
	}

	void action_auto_size(lv_event_t* e) {
		const uint32_t w = 1;
		if (lv_scr_act() == objects.d_chart_calib_on) {
			check_max_min();
			on_green_max = m_m.on.s_green.max + w;
			on_green_min = m_m.on.s_green.min - w;
			on_red_max = m_m.on.s_red.max + w;
			on_red_min = m_m.on.s_red.min - w;
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_PRIMARY_Y, (int32_t)on_green_min, (int32_t)on_green_max);
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_SECONDARY_Y, (int32_t)on_red_min, (int32_t)on_red_max);
		}
		else if (lv_scr_act() == objects.d_chart_calib_off) {
			check_max_min();
			off_green_max = m_m.off.s_green.max + w;
			off_green_min = m_m.off.s_green.min - w;
			off_red_max = m_m.off.s_red.max + w;
			off_red_min = m_m.off.s_red.min - w;
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_PRIMARY_Y, (int32_t)off_green_min, (int32_t)off_green_max);
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_SECONDARY_Y, (int32_t)off_red_min, (int32_t)off_red_max);
		}
		else if (lv_scr_act() == objects.d_chart_manual_edit_on) {
			check_max_min();
			if (col_but == green) {
				on_green_max = m_m.on.s_green.max + w;
				on_green_min = m_m.on.s_green.min - w;
			}
			else if (col_but == red) {
				on_red_max = m_m.on.s_red.max + w;
				on_red_min = m_m.on.s_red.min - w;
			}
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_PRIMARY_Y, (int32_t)on_green_min, (int32_t)on_green_max);
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_SECONDARY_Y, (int32_t)on_red_min, (int32_t)on_red_max);
		}
		else if (lv_scr_act() == objects.d_chart_manual_edit_off) {
			check_max_min();
			if (col_but == green) {
				off_green_max = m_m.off.s_green.max + w;
				off_green_min = m_m.off.s_green.min - w;
			}
			else if (col_but == red) {
				off_red_max = m_m.off.s_red.max + w;
				off_red_min = m_m.off.s_red.min - w;
			}
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_PRIMARY_Y, (int32_t)off_green_min, (int32_t)off_green_max);
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_SECONDARY_Y, (int32_t)off_red_min, (int32_t)off_red_max);
		}
		else if (lv_scr_act() == objects.d_chart_graph_resize_on) {
			if (col_but == green) {
				if (top_bot == top) {
					on_green_max = m_m.on.s_green.max + w;
				}
				else if (top_bot == bot) {
					on_green_min = m_m.on.s_green.min - w;
				}
			}
			else if (col_but == red) {
				if (top_bot == top) {
					on_red_max = m_m.on.s_red.max + w;
				}
				else if (top_bot == bot) {
					on_red_min = m_m.on.s_red.min - w;
				}
			}
			check_max_min();
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_PRIMARY_Y, (int32_t)on_green_min, (int32_t)on_green_max);
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_SECONDARY_Y, (int32_t)on_red_min, (int32_t)on_red_max);
		}
		else if (lv_scr_act() == objects.d_chart_graph_resize_off) {
			if (col_but == green) {
				if (top_bot == top) {
					off_green_max = m_m.off.s_green.max + w;
				}
				else if (top_bot == bot) {
					off_green_min = m_m.off.s_green.min - w;
				}

			}
			else if (col_but == red) {
				if (top_bot == top) {
					off_red_max = m_m.off.s_red.max + w;
				}
				else if (top_bot == bot) {
					off_red_min = m_m.off.s_red.min - w;
				}
			}
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_PRIMARY_Y, (int32_t)off_green_min, (int32_t)off_green_max);
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_SECONDARY_Y, (int32_t)off_red_min, (int32_t)off_red_max);
		}
		check_max_min();
		lv_chart_refresh(cur_shart);
	}

	void action_max_size_chart(lv_event_t* e) {
		if (lv_scr_act() == objects.d_chart_graph_resize_on) {
			if (col_but == green) {
				on_green_max = 4095;
				on_green_min = 0;
			}
			else if (col_but == red) {
				on_red_max = 4095;
				on_red_min = 0;
			}
			check_max_min();
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_PRIMARY_Y, (int32_t)on_green_min, (int32_t)on_green_max);
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_SECONDARY_Y, (int32_t)on_red_min, (int32_t)on_red_max);
		}
		else if (lv_scr_act() == objects.d_chart_graph_resize_off) {
			if (col_but == green) {
				off_green_max = 4095;
				off_green_min = 0;
			}
			else if (col_but == red) {
				off_red_max = 4095;
				off_red_min = 0;
			}
			check_max_min();
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_PRIMARY_Y, (int32_t)off_green_min, (int32_t)off_green_max);
			lv_chart_set_axis_range(cur_shart, LV_CHART_AXIS_SECONDARY_Y, (int32_t)off_red_min, (int32_t)off_red_max);
		}
		lv_chart_refresh(cur_shart);
	}

	void action_set_all(lv_event_t* e) { // TODO << ???
		LL_TIM_DisableCounter(TIM1); // PWM - tim clk
		LL_USART_DisableDMAReq_RX(UART5);
		all_H7_to_g4();
		LL_USART_EnableDMAReq_RX(UART5);
		LL_TIM_EnableCounter(TIM1); // PWM - tim clk
	}

	void action_read_all(lv_event_t* e) {
		LL_TIM_DisableCounter(TIM1); // PWM - tim clk
		LL_USART_DisableDMAReq_RX(UART5);
		all_g4_to_H7();
		LL_USART_EnableDMAReq_RX(UART5);
		LL_TIM_EnableCounter(TIM1); // PWM - tim clk
	}

	void action_calib_all(lv_event_t* e) { // TODO удалить кнопку
		// if (calib_all_OnOff != calib_on) {
		// 	LL_TIM_DisableCounter(TIM1); // PWM - tim clk
		// 	calib_all_OnOff = calib_on;
		// 	calib_all_str = "calibration..";
		// }
		// else {
		// 	calib_all_OnOff = calib_off;
		// 	calib_all_str = "calib cycle";
		// 	LL_TIM_EnableCounter(TIM1); // PWM - tim clk
		// }
	}

	void action_add_1(lv_event_t* e) {
		chart_correction(1, plus);
	}

	void action_add_10(lv_event_t* e) {
		chart_correction(10, plus);
	}

	void action_add_100(lv_event_t* e) {
		chart_correction(100, plus);
	}

	void action_add_1000(lv_event_t* e) {
		chart_correction(1000, plus);
	}

	void action_sub_1(lv_event_t* e) {
		chart_correction(1, minus);
	}

	void action_sub_10(lv_event_t* e) {
		chart_correction(10, minus);
	}

	void action_sub_100(lv_event_t* e) {
		chart_correction(100, minus);
	}

	void action_sub_1000(lv_event_t* e) {
		chart_correction(1000, minus);
	}

	void action_div_add_100(lv_event_t* e) {
		divis += 100000;
	}

	void action_div_add_1000(lv_event_t* e) {
		divis += 1000000;
	}

	void action_div_add_10000(lv_event_t* e) {
		divis += 10000000;
	}

	void action_div_add_100000(lv_event_t* e) {
		divis += 100000000;
	}

	void action_div_sub_100(lv_event_t* e) {
		divis -= 100000;
	}

	void action_div_sub_1000(lv_event_t* e) {
		divis -= 1000000;
	}

	void action_div_sub_10000(lv_event_t* e) {
		divis -= 10000000;
	}

	void action_div_sub_100000(lv_event_t* e) {
		divis -= 100000000;
	}
	//---------------------------------

	// LVGL VARS
	const char* get_var_l() {
		return l.c_str();
	}

	void set_var_l(const char* value) {
		l = value;
	}
	const char* get_var_r() {
		return r.c_str();
	}

	void set_var_r(const char* value) {
		r = value;
	}

	const char* get_var_ch_o() {
		return ch_o.c_str();
	}
	void set_var_ch_o(const char* value) {
		ch_o = value;
	}

	const char* get_var_ch_f() {
		return ch_f.c_str();
	}
	void set_var_ch_f(const char* value) {
		ch_f = value;
	}

	const char* get_var_s1_on_min() {
		return s1_on_min.c_str();
	}
	void set_var_s1_on_min(const char* value) {
		s1_on_min = value;
	}

	const char* get_var_s1_on_max() {
		return s1_on_max.c_str();
	}
	void set_var_s1_on_max(const char* value) {
		s1_on_max = value;
	}

	const char* get_var_s2_on_min() {
		return s2_on_min.c_str();
	}
	void set_var_s2_on_min(const char* value) {
		s2_on_min = value;
	}

	const char* get_var_s2_on_max() {
		return s2_on_max.c_str();
	}
	void set_var_s2_on_max(const char* value) {
		s2_on_max = value;
	}

	const char* get_var_s1_off_min() {
		return s1_off_min.c_str();
	}
	void set_var_s1_off_min(const char* value) {
		s1_off_min = value;
	}

	const char* get_var_s1_off_max() {
		return s1_off_max.c_str();
	}
	void set_var_s1_off_max(const char* value) {
		s1_off_max = value;
	}

	const char* get_var_s2_off_min() {
		return s2_off_min.c_str();
	}
	void set_var_s2_off_min(const char* value) {
		s2_off_min = value;
	}

	const char* get_var_s2_off_max() {
		return s2_off_max.c_str();
	}
	void set_var_s2_off_max(const char* value) {
		s2_off_max = value;
	}

	const char* get_var_sensor_on_1_data_string() {
		return sensor_on_1_data_string.c_str();
	}
	void set_var_sensor_on_1_data_string(const char* value) {
		sensor_on_1_data_string = value;
	}

	const char* get_var_sensor_on_2_data_string() {
		return sensor_on_2_data_string.c_str();
	}
	void set_var_sensor_on_2_data_string(const char* value) {
		sensor_on_2_data_string = value;
	}

	const char* get_var_sensor_off_1_data_string() {
		return sensor_off_1_data_string.c_str();
	}
	void set_var_sensor_off_1_data_string(const char* value) {
		sensor_off_1_data_string = value;
	}

	const char* get_var_sensor_off_2_data_string() {
		return sensor_off_2_data_string.c_str();
	}
	void set_var_sensor_off_2_data_string(const char* value) {
		sensor_off_2_data_string = value;
	}

	const char* get_var_divisible_eez_string() {
		 // TODO убрать код отсюда..
		divisible_eez_string = std::to_string(divis);
		return divisible_eez_string.c_str();
	}
	void set_var_divisible_eez_string(const char* value) {
		divisible_eez_string = value;
	}

	const char* get_var_cursor_string() {
		return cursor_string.c_str();
	}
	void set_var_cursor_string(const char* value) {
		cursor_string = value;
	}

	const char* get_var_disp_on_off_button() {
		return disp_on_off_button.c_str();
	}
	void set_var_disp_on_off_button(const char* value) {
		disp_on_off_button = value;
	}

	const char* get_var_disp_on_off_button_3() {
		return disp_on_off_button_3.c_str();
	}
	void set_var_disp_on_off_button_3(const char* value) {
		disp_on_off_button_3 = value;
	}

	const char* get_var_top_bot_str() {
		return top_bot_str.c_str();
	}
	void set_var_top_bot_str(const char* value) {
		top_bot_str = value;
	}

	const char* get_var_top_bot_str_2() {
		return top_bot_str_2.c_str();
	}
	void set_var_top_bot_str_2(const char* value) {
		top_bot_str_2 = value;
	}

	const char* get_var_debugg() {
		return debugg.c_str();
	}
	void set_var_debugg(const char* value) {
		debugg = value;
	}

	//for test
	const char* get_var_test_timer2() {
		return test_timer2.c_str();
	}
	void set_var_test_timer2(const char* value) {
		test_timer2 = value;
	}

	//for test
	const char* get_var_test_t_out_fl() {
		return test_t_out_fl.c_str();
	}
	void set_var_test_t_out_fl(const char* value) {
		test_t_out_fl = value;
	}

	//for test
	const char* get_var_test_speed_fl() {
		return test_speed_fl.c_str();
	}
	void set_var_test_speed_fl(const char* value) {
		test_speed_fl = value;
	}

	//for test
	const char* get_var_test_energy_fl() {
		return test_energy_fl.c_str();
	}
	void set_var_test_energy_fl(const char* value) {
		test_energy_fl = value;
	}

	//for test
	const char* get_var_test_midi_hi_fl() {
		return test_midi_hi_fl.c_str();
	}
	void set_var_test_midi_hi_fl(const char* value) {
		test_midi_hi_fl = value;
	}

	//for test
	const char* get_var_test_midi_lo_fl() {
		return test_midi_lo_fl.c_str();
	}
	void set_var_test_midi_lo_fl(const char* value) {
		test_midi_lo_fl = value;
	}

	// for test
	const char* get_var_note() {
		return note.c_str();
	}
	void set_var_note(const char* value) {
		note = value;
	}

	const char* get_var_calib_all_str() {
		return calib_all_str.c_str();
	}
	void set_var_calib_all_str(const char* value) {
		calib_all_str = value;
	}

	const char* get_var_mass_str() {
		return mass_str.c_str();
	}
	void set_var_mass_str(const char* value) {
		mass_str = value;
	}

	const char* get_var_t1() {
		return t1.c_str();
	}
	void set_var_t1(const char* value) {
		t1 = value;
	}

	const char* get_var_t2() {
		return t2.c_str();
	}
	void set_var_t2(const char* value) {
		t2 = value;
	}

	const char* get_var_t3() {
		return t3.c_str();
	}
	void set_var_t3(const char* value) {
		t3 = value;
	}

	const char* get_var_timer_data() {
		return timer_data.c_str();
	}
	void set_var_timer_data(const char* value) {
		timer_data = value;
	}

	const char* get_var_chart_calib_online() {
		return chart_calib_online.c_str();
	}
	void set_var_chart_calib_online(const char* value) {
		chart_calib_online = value;
	}
}
#endif // extern "C"
