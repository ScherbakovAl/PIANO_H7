/*
 * midi_keyboard_h7-2.cpp
 *
 *  Created on: Apr 8, 2025
 *      Author: sche
 */

#include "vector"
#include "map"
#include <ranges>
#include <format>
#include "piano_h7.hpp"

// inline constexpr uint32_t NOTE_OFFSETS[196] = {
//     // Индексы для коррекции MIDI номера
//     // Эти значения зависят от конкретной клавиатуры
//     [0 ... 53] = 14,    // 0-54: +14
//     [54 ... 95] = 13,   // 55-95: +13
//     [96 ... 145] = -77, // 96-145: -77
//     [146 ... 195] = -78 // 146+: -78
// };


void pwr() {
	//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv управление питанием
	if (!__HAL_PWR_GET_FLAG(PWR_FLAG_SB)) {
		HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN4); //pin4 == кнопка К1 на плате
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
		HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4);
		LCD_WR_REG(0x10);
		// LCD_WR_REG(0x28); // DISPOFF (28h): Display Off
		HAL_PWR_EnterSTANDBYMode();
	}
	else {
		// GPIOA->BSRR |= 0x20; // for test // DEBUG
		HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN4);
		// GPIOA->BSRR |= 0x200000;
		GPIOD->BSRR = 0x40;// pD6 - LED подсветка
	}
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ управление питанием
}

void to_sleep() {
	// LCD_stby();
	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN4);
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
	HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4);
	HAL_PWR_EnterSTANDBYMode();
}

void init() {

	// GPIOD->BSRR = 0x40;// pD6 - LED подсветка
	// GPIOD->BSRR = 0x400000;// pD6 - LED подсветка

	// pause(2);
	// LL_mDelay(120);
	// pwr();
	// tud_disconnect(); // TODO tud_disconnect() // это работает
	// GPIOD->BSRR = 0x400000;// pD6 - LED подсветка

	init_LL();
	init_LCD_touch();
	lv_init();
	disp_start();
	touch_start();
	ui_init();
	tusb_init();

	tud_task();
	lv_timer_handler();
	ui_tick();

	// pause(10);
	// send_test_midi();

	LL_TIM_DisableCounter(TIM1);  // PWM - tim clk

	initBuffers();
	configCharts();

	// память
	// SaveToMemory();
	// ReadOnMemory(); // восстановление графика при включении
	// debugg_fn("   -- -- Restore Calib DONE! -- --)"); // DEBUG
	// LL_TIM_DisableCounter(TIM1); // PWM - tim clk
	// LL_USART_DisableDMAReq_RX(UART5);
	// all_H7_to_g4();
	// LL_USART_EnableDMAReq_RX(UART5);
	// LL_TIM_EnableCounter(TIM1); // PWM - tim clk
	// debugg_fn("   -- H7 > >>>> > G4 DONE! --)"); // DEBUG
	//---------------------------------
	// debugg_fn(""); // DEBUG
	// debugg_fn(""); // DEBUG
	// debugg_fn(">>>  HELLOO tit !  <<<<"); // DEBUG
	// debugg_fn(""); // DEBUG

	// tud_task();
	// lv_timer_handler();
	// ui_tick();

	// GPIOA->BSRR = 0x10; // for test // DEBUG
	// GPIOA->BSRR = 0x100000;
	// GPIOA->BSRR = 0x20; // for test // DEBUG
	// GPIOA->BSRR = 0x200000;
	// start PWM

	// LL_TIM_EnableCounter(TIM1); // PWM - tim  - не стартуют чипы если выкл
	//---------------------------------



	// pause(5); // DEBUG
	// debugg_clear();
	// debug_counter = 1;
	// int test_int_timer2_old = test_int_timer2;

	LL_USART_DisableDMAReq_RX(UART5);

	init_chips();

	if (chip_state == chip_states::boot) {
		jump_g4s_to_adress();
		pause(50000);
	}

	all_H7_to_g4();

	sync(); // включает прерывания и таймер, осторожно!

	// LL_USART_EnableDMAReq_RX(UART5); // это уже есть внутри sync();
	// LL_TIM_EnableCounter(TIM1); // PWM - tim clk

}

void init_LL() {

	// TIM init
	LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);
	LL_TIM_EnableAllOutputs(TIM1); // PWM - tim clk
	LL_TIM_EnableIT_TRIG(TIM1);

	LL_TIM_EnableCounter(TIM2); // просто счётчик (275Mhz)

	LL_TIM_SetAutoReload(TIM3, allChipCount);
	LL_TIM_EnableCounter(TIM3); // считает номер контроллера g4

	LL_TIM_EnableCounter(TIM4); // для LVGL
	LL_TIM_EnableIT_UPDATE(TIM4);

	LL_TIM_EnableCounter(TIM5); // ограничение скорости сканирования плат

	// UART init
	LL_USART_Enable(UART5);
	LL_USART_EnableDMAReq_RX(UART5);

	// DMA RX для получения данных
	LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_2);
	LL_DMA_SetPeriphAddress(DMA1, LL_DMA_STREAM_2, (uint32_t) & (UART5->RDR));
	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_2, (uint32_t)rx_data);
	LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_2, dataLengthRX);
	LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_2); // включает прерывание transfer complete
	LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_2);

}

void init_LCD_touch() {

	// LCD init
	LL_SPI_Enable(SPI3);
	LL_SPI_StartMasterTransfer(SPI3);
	LCD_Init();

	// TOUCH init
	// LL_I2C_Enable(I2C5);
	FT6336_Init();

}

void disp_start() {

	// DISP start
	disp = lv_display_create(320, 480);
	lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB888);
	lv_display_set_flush_cb(disp, my_flush_cb);
	lv_display_set_buffers(disp, buf_1, buf_2, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
	// lv_display_set_flush_wait_cb(disp, my_flush_wait);

}

void touch_start() {

	// TOUCH start
	LL_TIM_EnableIT_UPDATE(TIM6); // для TOUCH
	indev = lv_indev_create();
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, my_input_read);

}

void init_chips() {
	vChips.clear();
	vChips.reserve(allChipCount);
	mComparatorCursor_on.clear();
	mComparatorCursor_off.clear();

	for (int i = 0; i < rx_settings_length; ++i) {
		rx_settings[i] = 0;
	}

	std::string strOut = "ships .. ";
	std::string stat;

	for (uint8_t chip = 0; chip < allChipCount; ++chip) {
		rx_settings[0] = 0;
		UART4_SendAddress(chip);
		Set_tx_s((uint8_t)bootloader_command::echo, 0, 0, 0, 0);
		UART4_Send_Settings_bootloader();
		UART4_Receive_timeout_10us();
		if (rx_settings[1] != 0 || rx_settings[2] != 0 || rx_settings[3] != 0) { // проверка, что приняты " aadr 0 0 0 state"
			strOut += "\n  * * NOISE!!! * *  ";
		}
		chip_state = (chip_states)rx_settings[4];

		if (rx_settings[0]) {

			uint8_t cursor = 0;
			uint8_t addr = 0;
			std::vector<comparator> vComparators;
			vComparators.reserve(count_comparators);

			for (uint8_t comp = 0; comp < count_comparators; ++comp) {
				if (chip < on_off_division) {
					cursor = (chip * count_comparators) + comp;
					addr = cursor;
					mComparatorCursor_on.emplace(cursor, comparator(cursor, addr, chip, comp, true));
				}
				else {
					cursor = ((chip - on_off_division) * count_comparators) + comp;
					addr = cursor + buffer_division;
					mComparatorCursor_off.emplace(cursor, comparator(cursor, addr, chip, comp, true));
				}
				vComparators.push_back(comparator(cursor, addr, chip, comp, true));
			}
			vChips.push_back({ chip, vComparators, (chip < on_off_division ? typeAction::on : typeAction::off), chip_state });

			strOut += std::format(" {}", chip);
		}
	}


	if (chip_state == chip_states::boot) {
		stat = " bootloaders";
	}
	else {
		stat = " pianos";
	}
	strOut += stat;

	debugg_fn(strOut);

	cursor = vChips.front().comparators.front().address; // TODO установить края отображаемого графика
}

// добавить анимацию: https://duino.ru/blog/onlayn-konverter-gif-animatsii-v-iskhodnyy-kod-dlya-arduino/
void h7() {

	init();

	while (1) {

		tud_task();
		lv_timer_handler_run_in_period(5);
		ui_tick();

		if (TIM5->CNT > 3000) { // 1000 = 1ms (чтобы калибровка не наступала себе на пятки)

			if (cur_disp == current_display::on) {

				//  Элементы с 3-го по 7-й (индексы 3-6)
				//  for (auto n : v | std::views::drop(3) | std::views::take(4)) 
				for (const auto& chip : vChips) { // TODO это не то, что я ожидаю...не "пропустить" в количестве "on_off_division"
					if (chip.typ == typeAction::on) {
						sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::read_calibration);
						for (const auto& comp : chip.comparators) {
							UART4_Receive_Settings();
							buffer_calib[comp.address] = convert_8_16(a_, b_);
						}
						refresh_cursor(chip); // TODO refresh_cursor переделать нормально
					}
				}
				chart_calib_online = std::to_string(buffer_calib[cursor]);
				l = std::to_string(buffer_calib[cursor - 1]);
				r = std::to_string(buffer_calib[cursor + 1]);
				lv_chart_set_cursor_point(objects.chart_on, cursor_on_hor, ser_on_blue, cursor);
				lv_chart_refresh(cur_shart);
			}

			if (cur_disp == current_display::off) {

				for (const auto& chip : vChips) {  // TODO это не то, что я ожидаю...не "пропустить" в количестве "on_off_division"
					if (chip.typ == typeAction::off) {
						sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::read_calibration);
						for (const auto& comp : chip.comparators) {
							UART4_Receive_Settings();
							buffer_calib[comp.address] = convert_8_16(a_, b_);
						}
						refresh_cursor(chip); // TODO refresh_cursor переделать нормально
					}
				}
				chart_calib_online = std::to_string(buffer_calib[cursor]);
				l = std::to_string(buffer_calib[cursor - 1]);
				r = std::to_string(buffer_calib[cursor + 1]);
				lv_chart_set_cursor_point(objects.chart_off, cursor_off_hor, ser_off_blue, cursor);
				lv_chart_refresh(cur_shart);
			}

			TIM5->CNT = 0;

		}

		if (fl) { // DEBUG

			// tud_disconnect();
			// debugg_fn("usb disconnect!")

			debugg_fn(std::format("  .   .    .   . . "));
			debugg_fn(std::format("tx = {:#04x} - {:#04x} - {:#04x} - {:#04x} - {:#04x}", tx_settings[0], tx_settings[1], tx_settings[2], tx_settings[3], tx_settings[4]));
			debugg_fn(std::format("rx = {:#04x} - {:#04x} - {:#04x} - {:#04x} - {:#04x}", rx_settings[0], rx_settings[1], rx_settings[2], rx_settings[3], rx_settings[4]));

			// debugg_fn(std::format("tOut = { :.5f }", rx_settings[1]));
			// debugg_fn(std::format("rx_settings = {:}", rx_settings[1]));
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
	}
} // h7

void sync() { // включает прерывания, осторожно!
	LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
	LL_USART_DisableDMAReq_RX(UART5);

	TIM3->CNT = 0; // сбросить номер контроллера
	int fl_sync = 0; // for test
	for (const auto& chip : vChips) {
		fl_sync += sync_sender(chip.number_chip);
	}
	if (fl_sync) {
		debugg_fn(std::format("Sync {} bugs", fl_sync));
	}
	else {
		debugg_fn("Sync OK");
	}

	LL_USART_EnableDMAReq_RX(UART5);
	LL_TIM_EnableCounter(TIM1);  // PWM - tim clk
}

int sync_sender(const uint8_t& i) {
	int fs = 0;
	UART4_SendAddress(i);
	pause(3);
	UART4_Send_Settings(command::sync_timer, 0, 0, 0);
	// pause(1);
	// UART4_Receive_Settings();
	if (UART4_Receive_timeout_10us()) {
		debugg_fn("UART timeout " + std::to_string(i));
	}
	if (b_ != 0 && a_ != 0 && rx_settings[0] != i) {
		debugg_fn("Sync err, mcu  #" + std::to_string(i));
		++fs;
	}
	pause(1);
	return fs;
}

void check_max_min() { // TODO deprecated
	on_off_s1_s2_min_max mm; //  для сброса состояния max_min
	m_m = mm;
	if (cur_disp == current_display::on) {
		for (int i = 0; i < 98; ++i) {
			if (m_m.on.s_green.min > compsCHART_green[i]) {
				m_m.on.s_green.min = compsCHART_green[i];
			}
			if (m_m.on.s_green.max < compsCHART_green[i]) {
				m_m.on.s_green.max = compsCHART_green[i];
			}
			if (m_m.on.s_red.min > compsCHART_red[i]) {
				m_m.on.s_red.min = compsCHART_red[i];
			}
			if (m_m.on.s_red.max < compsCHART_red[i]) {
				m_m.on.s_red.max = compsCHART_red[i];
			}
		}

		s1_on_min = std::to_string(m_m.on.s_green.min);
		s1_on_max = std::to_string(m_m.on.s_green.max);
		s2_on_min = std::to_string(m_m.on.s_red.min);
		s2_on_max = std::to_string(m_m.on.s_red.max);
	}
	if (cur_disp == current_display::off) {
		for (int i = 98; i < 196; ++i) {
			if (m_m.off.s_green.min > compsCHART_green[i]) {
				m_m.off.s_green.min = compsCHART_green[i];
			}
			if (m_m.off.s_green.max < compsCHART_green[i]) {
				m_m.off.s_green.max = compsCHART_green[i];
			}
			if (m_m.off.s_red.min > compsCHART_red[i]) {
				m_m.off.s_red.min = compsCHART_red[i];
			}
			if (m_m.off.s_red.max < compsCHART_red[i]) {
				m_m.off.s_red.max = compsCHART_red[i];
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

	for (const auto& chip : vChips) {
		for (const auto& comp : chip.comparators) {
			sender(command::set_comp_value, chip.number_chip, comp.number_comparator, dot::green, buffer_green[comp.address]);
			sender(command::set_comp_value, chip.number_chip, comp.number_comparator, dot::red, buffer_red[comp.address]);
			if (chip.typ == typeAction::off) {
				sender(command::set_comp_value, chip.number_chip, comp.number_comparator, dot::grey, buffer_green[comp.address] - (buffer_green[comp.address] / 10));
			}
		}
	}
}

void all_g4_to_H7() {
	pause(10); // если вдруг кто-то захочет что-то отправить... ?

	for (const auto& chip : vChips) {
		for (const auto& comp : chip.comparators) {
			if (chip.typ == typeAction::on) {
				sender(command::read_comp_value, chip.number_chip, comp.number_comparator, dot::green, 0);
				buffer_green[comp.address] = convert_8_16(a_, b_);
				sender(command::read_comp_value, chip.number_chip, comp.number_comparator, dot::red, 0);
				buffer_red[comp.address] = convert_8_16(a_, b_);
			}
		}
	}
}

void refresh_cursor(const Chip& chip) { // TODO refresh cursor пересобрать
	bool fl_c = false;
	for (const auto& comp : chip.comparators) {
		const auto& addr = comp.address;
		if (buffer_calib[addr] < 4080 && buffer_calib[addr] > 2) {

			if (buffer_calib_old[addr] + 200 < buffer_calib[addr]) {
				buffer_calib_old[addr] = buffer_calib[addr];
				fl_c = true;
			}
			else if (buffer_calib_old[addr] - 200 > buffer_calib[addr]) {
				buffer_calib_old[addr] = buffer_calib[addr];
				fl_c = true;
			}
		}

		if (fl_c) {
			cursor = comp.cursor;
			if (addr > buffer_division) {
				lv_chart_set_cursor_point(objects.chart_off, cursor_off_vert, ser_off_green, cursor);
				sensor_off_1_data_string = std::to_string(buffer_green[addr]);
				sensor_off_2_data_string = std::to_string(buffer_red[addr]);
				cursor_string = std::to_string(cursor);
			}
			else {
				lv_chart_set_cursor_point(objects.chart_on, cursor_on_vert, ser_on_green, cursor);
				sensor_on_1_data_string = std::to_string(buffer_green[addr]);
				sensor_on_2_data_string = std::to_string(buffer_red[addr]);
				cursor_string = std::to_string(cursor);
			}
			fl_c = false;
		}
	}
}

void sender(const command& com, const uint8_t& adress, const uint8_t& compN, const dot& dot, const uint32_t& value) {
	UART4_SendAddress(adress);
	pause(3); // 4 for release, 10-debug g4
	UART4_Send_Settings(com, compN, (uint8_t)dot, value);
	UART4_Receive_Settings();
	pause(1); // 1 for release, 4-debug g4
	if (adress != rx_settings[0]) {
		debugg_fn(std::format("BAD ADRESS {}   rx_settings {}", adress, rx_settings[0]));
	}
	if (com == command::set_comp_value) {
		if (convert_8_16(a_, b_) != value) {
			debugg_fn(std::format("BAD SET DATA [0]= {}, add={}, comp={}, dot={}, value={}, in={}", rx_settings[0], adress, compN, (uint8_t)dot, value, convert_8_16(a_, b_)));
		}
	}
}

// UART Send-Recive
void UART4_SendAddress(const uint8_t& slave_address) {
	const uint16_t address_byte = slave_address | 0x100; // Установка старшего бита (MSB) для указания адреса
	while (!LL_USART_IsActiveFlag_TXE(UART5)) {}
	LL_USART_TransmitData9(UART5, address_byte);
	while (!LL_USART_IsActiveFlag_TC(UART5)) {}
	pause(2);
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

void UART4_Send_Settings_bootloader() { // tx_settings[0] - [4]
	while (!LL_USART_IsActiveFlag_TXE(UART5)) {}
	for (uint16_t i = 0; i < tx_settings_length; i++) {
		LL_USART_TransmitData9(UART5, tx_settings[i]);
		while (!LL_USART_IsActiveFlag_TXE(UART5)) {}
	}
	while (!LL_USART_IsActiveFlag_TC(UART5)) {}
}

void UART4_Receive_Settings_bootloader() { // rx_settings[0] - [4]
	for (uint8_t i = 0; i < rx_settings_length; i++) {
		while (!LL_USART_IsActiveFlag_RXNE(UART5)) {}
		rx_settings[i] = (uint8_t)LL_USART_ReceiveData9(UART5);
	}
}

int UART4_Receive_timeout_10us() {
	for (uint8_t i = 0; i < rx_settings_length; i++) { // Таймаут 10us (275 тиков таймера TIM2 на 275MHz)
		TIM2->CNT = 0;
		const uint32_t timeout = 15 * 275; // 10us * 275 тиков/us

		while (!LL_USART_IsActiveFlag_RXNE(UART5)) {
			if (TIM2->CNT >= timeout) { // Время ожидания истекло, выходим без чтения
				return 1;
			}
		}
		rx_settings[i] = (uint8_t)LL_USART_ReceiveData9(UART5);
	}
	return 0;
}


const float key_mass = 0.008f; // 8 гр -->> переехал в массив
const float distance_F = 0.0017f; // 1.7 мм (толщина шаблонов 1.9 и 0.2)
const float div_on = 0.000000000092f; // меньше - громче
const float div_off = 0.00000000004f; // меньше - громче 
const float deriv_F = 2.0f; // делить на 2 в формуле
const float maxMidi_F = 127.99f;

const float key_mass_2 = 819.79f;
const float distance_2 = 17000.0f;
const float aX = 24568.0f;
const float aY = -1.2f;

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

// если
// тут
// ошибки 
// ..
// (определить условия, при которых сюда зайдёт..)
//
// то
// поднимаем флаг тут
// и 
// в основном цикле
// пишем сообщение об ошибке
// и
// сбрасываем чипы
// + синхронизируем заново
// + обновляем информацию в компараторах и массивах со значениями калибровки



		// 	USART_Noise_Error_detected(); // DEBUG
		// 	debugg_fn("... t > 3");
		// }

		uint32_t tOut = 0;
		tOut = rx_data[1] << 16 | rx_data[2] << 8 | rx_data[3];
		float integerPart_F;
		int note_ = rxB + noteAdder[rxB];

#define speee
#ifdef speee

		// if (rxB < 98) {
		// 	timerLenght_F = (float)tOut * div_on;
		// }
		// else {
		// 	timerLenght_F = (float)tOut * div_off;
		// }

		// //var 1
		// speed_F = distance_F / timerLenght_F;
		// energy_F = (mass_F[rxB] * speed_F * speed_F) / deriv_F;
		// midi_hi_F = energy_F / maxMidi_F;
		// midi_lo_F = modf(midi_hi_F, &integerPart_F) * maxMidi_F;


		// VAR 2
		speed_F = distance_2 / (tOut + aX);
		energy_F = (key_mass_2 * speed_F * speed_F) / 2.0f;
		midi_hi_F = energy_F;
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

		if (tOut < 5664) tOut = 5664;

		//47.9 + 51.23(17000)-2474.3
		midi_hi_F = 66.2f + (63.88f * log10f(17000.0f / ((float)tOut - 3764.0f))); // 74 + 78? // 57.96 + 100? // 57.96 + 71.3? // 70 + 74(17000)?
		midi_lo_F = modf(midi_hi_F, &integerPart_F) * maxMidi_F;
		note_ = rxB + noteAdder[rxB];

		if (midi_hi_F < 1) {
			midi_hi_F = 1;
			midi_lo_F = 1;
		}

		// if (midi_hi_F > 127) {
		// 	midi_hi_F = 127;
		// 	midi_lo_F = 127;
		// }

		uint8_t note_buf2[] = {
			0xB0,
			0x58,
			(uint8_t)midi_lo_F,
			rxB < 98 ? 0x90 : 0x80, // 0x90 note on
			note_,
			(uint8_t)midi_hi_F
		};

		tud_midi_stream_write(0, note_buf2, 6);

		// fl = rxB < 98 ? 1 : 0; // for test // разрешить обновлять цифры на дисплее
		// timerLenght_F = midi_hi_F; // DEBUG
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

// пометочки - вычисление скорости молоточка
// tim_IN = 100ns на значение
// 35445 = 22.211 midi
//
// v = s / t
// s - расстояние
// s = 2 mm = 0.002 m
// t - время
// t = 62000 ns = 0.000062 s
//
// v = 0.002 / 0.000062 = 32,258064516 м/с;
// v = 2000 / 62 = 32,258064516; ~~~
// v = 2000000 / 62000 = 32,258064516;
//
// A - кинетическая энергия
// A = M * v * v / 2;
//
// М - масса (кг)
// M = 8 g = 0.008 kg
// v * v - скорость в квадрате (м/с)
//
// A = 0.02 * 32,258064516 * 32,258064516 / 2 = 10,405827263;
//---------------------------------

void initBuffers() {
	// for (int i = 0; i < 196; ++i) { // TODO deprecated
	// 	if (i < 98) {
	// 		compsCHART_green[i] = def_on[0];
	// 		compsCHART_red[i] = def_on[1];
	// 	}
	// 	else {
	// 		compsCHART_green[i] = def_off[0];
	// 		compsCHART_red[i] = def_off[1];
	// 	}
	// 	compsCHART_CALIB[i] = 800;
	// }

	for (uint32_t i = 0; i < buffer_division; ++i) {
		buffer_green[i] = green_on_default;
		buffer_red[i] = red_on_default;
		buffer_calib[i] = 0;
	}
	for (uint32_t i = buffer_division; i < size_BUFFER; ++i) {
		buffer_green[i] = green_off_default;
		buffer_red[i] = red_off_default;
		buffer_calib[i] = 0;
	}

	for (uint i = 0; i < size_BUFFER; ++i) { // TODO note shift // TODO проверить здесь что происходит...
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

	for (uint32_t i = 0; i < size_BUFFER; ++i) {
		mass_F[i] = key_mass; //  + (float)i / 10000000; // 8 гр
	}
}

int32_t convert_8_16(const uint8_t& a, const uint8_t& b) {
	int32_t x = a << 8 | b;
	return x;
}

conv_16_8 convert_16_8(const uint32_t& a) {
	conv_16_8 result;
	// TODO AI - Проблема: Неправильный порядок операций. Должно быть r.a = (a & (0xff << 8)) >> 8; или r.a = (a >> 8) & 0xff;
	result.a = (a & 0xff << 8) >> 8;
	result.b = a & 0xff;
	return result;
}

void SaveToMemory() {

	SCB_DisableICache();
	SCB_DisableDCache();
	HAL_FLASH_Unlock();

	FLASH_Erase_Sector(FLASH_SECTOR_7, FLASH_BANK_1, FLASH_VOLTAGE_RANGE_2);

	uint32_t Addr = FLASH_ADDRESS;
	for (uint32_t i = 0; i < size_BUFFER; i += 8) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, Addr, (uint32_t)&buffer_green[i]) != HAL_OK) {
			HAL_FLASH_Lock();
			SCB_EnableICache();
			SCB_EnableDCache();
			debugg_fn(" FLASH err 1 ");
			return;
		}
		Addr += 0x20;
	}
	for (uint32_t i = 0; i < size_BUFFER; i += 8) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, Addr, (uint32_t)&buffer_red[i]) != HAL_OK) {
			HAL_FLASH_Lock();
			SCB_EnableICache();
			SCB_EnableDCache();
			debugg_fn(" FLASH err 2 ");
			return;
		}
		Addr += 0x20;
	}

	// // замок на запись (по адресу Flash_Address + 0x640)
	// if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, Addr, (uint32_t)&key_to_change_memory) != HAL_OK) {
	// 	HAL_FLASH_Lock();
	// 	return;
	// }

	HAL_FLASH_Lock();
	SCB_EnableICache();
	SCB_EnableDCache();

}

void ReadOnMemory() {
	// SCB_InvalidateDCache();
	// if ((*(volatile uint32_t*)(Flash_Address + 0x640)) != key_to_change_memory) {
	// 	SaveToMemory();
	// }
	// else {

	// Вычисляем смещение для buffer_red
	// buffer_green занимает: 200 элементов * 4 байта = 800 байт = 0x320
	// НО! При записи используется FLASHWORD (32 байта на 8 элементов)
	// 200 элементов / 8 = 25 блоков * 32 байта = 800 байт = 0x320
	const uint32_t offset_buffer_red = (size_BUFFER / 8) * 0x20; // = 25 * 32 = 800 = 0x320
	for (uint32_t i = 0; i < size_BUFFER; ++i) {
		buffer_green[i] = *(volatile uint32_t*)(FLASH_ADDRESS + (i * sizeof(uint32_t)));
		buffer_red[i] = *(volatile uint32_t*)(FLASH_ADDRESS + offset_buffer_red + (i * sizeof(uint32_t)));
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
	if (debug_counter % 27 == 0)debugg_clear();
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
	SCB_CleanInvalidateDCache();
	Send_DMA_Data8(color_p, wh_);
}

void configCharts() {
	lv_obj_t* ob = objects.chart_on;
	lv_chart_set_point_count(ob, buffer_division); // TODO подобрать значение
	ser_on_blue = lv_chart_add_series(ob, lv_color_hex(0x314ded), LV_CHART_AXIS_PRIMARY_X); // LV_COLOR_MAKE(0xE9, 0x1E, 0x63)
	ser_on_green = lv_chart_add_series(ob, lv_color_hex(0x0aaa37), LV_CHART_AXIS_PRIMARY_X);
	ser_on_red = lv_chart_add_series(ob, lv_color_hex(0xdb591e), LV_CHART_AXIS_PRIMARY_X);
	lv_chart_set_series_ext_y_array(ob, ser_on_green, buffer_green);
	lv_chart_set_series_ext_y_array(ob, ser_on_red, buffer_red);
	lv_chart_set_series_ext_y_array(ob, ser_on_blue, buffer_calib);
	lv_chart_set_axis_range(ob, LV_CHART_AXIS_PRIMARY_Y, on_green_max, on_green_min);
	lv_chart_set_axis_range(ob, LV_CHART_AXIS_SECONDARY_Y, on_red_max, on_red_min);
	cursor_on_vert = lv_chart_add_cursor(ob, lv_color_hex(0x808080), LV_DIR_VER);
	cursor_on_hor = lv_chart_add_cursor(ob, lv_color_hex(0x808080), LV_DIR_HOR);
	lv_chart_set_cursor_point(ob, cursor_on_vert, ser_on_green, cursor);
	lv_chart_set_cursor_point(ob, cursor_on_hor, ser_on_blue, cursor);
	lv_obj_set_style_line_width(ob, 1, LV_PART_CURSOR); // толщина курсора
	lv_obj_set_style_line_width(ob, 0, LV_PART_ITEMS);  // толщина линий между точками на графике
	lv_obj_set_style_size(ob, 2, 3, LV_PART_INDICATOR); // размер точек на графике
	lv_chart_set_div_line_count(ob, 0, 0);
	lv_obj_set_style_radius(ob, 0, 0);

	ob = objects.chart_off;
	lv_chart_set_point_count(ob, buffer_division); // TODO подобрать значение
	ser_off_blue = lv_chart_add_series(ob, lv_color_hex(0x314ded), LV_CHART_AXIS_PRIMARY_X);
	ser_off_green = lv_chart_add_series(ob, lv_color_hex(0x0aaa37), LV_CHART_AXIS_PRIMARY_X);
	ser_off_red = lv_chart_add_series(ob, lv_color_hex(0xdb591e), LV_CHART_AXIS_PRIMARY_X);
	lv_chart_set_series_ext_y_array(ob, ser_off_green, &buffer_green[buffer_division]);
	lv_chart_set_series_ext_y_array(ob, ser_off_red, &buffer_red[buffer_division]);
	lv_chart_set_series_ext_y_array(ob, ser_off_blue, &buffer_calib[buffer_division]);
	lv_chart_set_axis_range(ob, LV_CHART_AXIS_PRIMARY_Y, off_green_max, off_green_min);
	lv_chart_set_axis_range(ob, LV_CHART_AXIS_SECONDARY_Y, off_red_max, off_red_min);
	cursor_off_vert = lv_chart_add_cursor(ob, lv_color_hex(0x808080), LV_DIR_VER); // lv_color_make(200, 200, 200)
	cursor_off_hor = lv_chart_add_cursor(ob, lv_color_hex(0x808080), LV_DIR_HOR);
	lv_chart_set_cursor_point(ob, cursor_off_vert, ser_off_green, cursor);
	lv_chart_set_cursor_point(ob, cursor_off_hor, ser_off_blue, cursor);
	lv_obj_set_style_line_width(ob, 1, LV_PART_CURSOR); // толщина курсора
	lv_obj_set_style_line_width(ob, 0, LV_PART_ITEMS);  // толщина линий между точками на графике
	lv_obj_set_style_size(ob, 2, 3, LV_PART_INDICATOR); // размер точек на графике
	lv_chart_set_div_line_count(ob, 0, 0);
	lv_obj_set_style_radius(ob, 0, 0);
}

/**
 * @brief Чтение данных из Flash памяти в формате uint32_t
 * @param Address: Адрес начала чтения во Flash (должен быть выровнен по 4 байта)
 * @param pData: Указатель на массив uint32_t для сохранения прочитанных данных
 * @param Size: Размер данных в БАЙТАХ (не в элементах uint32_t!)
 * @note  Функция безопасна для вызова из прерывания
 * @note  Размер Size будет округлен вверх до кратного 4
 *
 * Пример использования:
 * __attribute__((aligned(32))) volatile uint32_t bin_data_32[512];
 * Read_uint32(0x08008000, bin_data_32, 2048); // Прочитать 2048 байт (512 uint32_t)
 */
void Read_uint32(uint32_t Address, volatile uint32_t* pData, uint32_t Size) {

	if (pData == NULL || Size == 0) {
		return;
	}

	// Проверка, что адрес находится в области флеш-памяти
	if (Address < FLASH_BASE || Address >= (FLASH_BASE + FLASH_SIZE)) {
		return;
	}

	// Проверка выравнивания адреса по 4 байта для оптимальной работы
	if (Address % 4 != 0) {
		return;
	}

	// Вычисляем количество uint32_t элементов (округляем вверх)
	uint32_t count = (Size + 3) / 4;

	// Проверка, что чтение не выходит за границы флеш-памяти
	if ((Address + (count * 4)) > (FLASH_BASE + FLASH_SIZE)) {
		return;
	}

	// Прямое чтение из памяти uint32_t словами (быстрее чем побайтно)
	volatile uint32_t* pFlashAddr = (volatile uint32_t*)Address;

	// Используем критическую секцию для безопасности вызова из прерывания
	uint32_t primask = __get_PRIMASK();
	// __disable_irq();

	for (uint32_t i = 0; i < count; i++) {
		pData[i] = pFlashAddr[i];
	}

	__set_PRIMASK(primask);
}

/**
 * @brief Запись данных в Flash память в формате uint32_t
 * @param Address: Адрес начала записи во Flash (должен быть выровнен по 8 байт)
 * @param Data: Указатель на массив uint32_t с данными для записи
 * @param size: Размер данных в БАЙТАХ (не в элементах uint32_t!)
 * @note  Функция безопасна для вызова из прерывания
 * @note  Размер size будет округлен вверх до кратного 8 (т.к. запись doubleword)
 * @note  Перед записью необходимо стереть страницу Flash с помощью Erase()
 *
 * Пример использования:
 * __attribute__((aligned(32))) volatile uint32_t bin_data_32[512] = {данные};
 * uint32_t addr = 0x08008000;
 * FlashPageInfo_t info = GetPageAndBank(addr, 2048);
 * Erase(info.start_page, info.bank);
 * Flash_uint32(addr, bin_data_32, 2048); // Записать 2048 байт (512 uint32_t)
 */
/*void Flash_uint32(uint32_t Address, volatile uint32_t* Data, uint32_t size) {
	// Проверка выравнивания адреса по 8 байт для DOUBLEWORD
	if (Address % 8 != 0) {
		return;
	}

	// Проверяем корректность параметров
	if (size == 0 || Data == NULL) {
		return;
	}

	// Используем критическую секцию для безопасности вызова из прерывания
	uint32_t primask = __get_PRIMASK();
	// __disable_irq();

	HAL_FLASH_Unlock();

	// Барьер памяти перед началом операций
	__DMB();

	// Вычисляем количество DOUBLEWORD для записи (каждое = 8 байт = 2 uint32_t)
	uint32_t doublewords_count = (size + 7) / 8;

	// Записываем данные в цикле
	for (uint32_t i = 0; i < doublewords_count; i++) {
		uint32_t current_address = Address + (i * 8);
		uint64_t current_data;

		// Индекс в массиве uint32_t (каждый doubleword = 2 uint32_t)
		uint32_t idx = i * 2;

		// Формируем 64-битное значение из двух 32-битных
		// Little Endian: младшие биты идут первыми
		if ((i * 8 + 8) <= size) {
			// Полный doubleword
			current_data = ((uint64_t)Data[idx]) | (((uint64_t)Data[idx + 1]) << 32);
		} else {
			// Последний неполный doubleword - заполняем FF
			current_data = 0xFFFFFFFFFFFFFFFF;
			uint32_t remaining_bytes = size - (i * 8);

			if (remaining_bytes >= 4) {
				// Первые 4 байта из Data[idx]
				current_data = (uint64_t)Data[idx];
				if (remaining_bytes > 4) {
					// Частично второй uint32_t
					uint32_t partial = Data[idx + 1];
					uint32_t valid_bytes = remaining_bytes - 4;
					uint32_t mask = (1UL << (valid_bytes * 8)) - 1;
					partial &= mask;
					current_data |= ((uint64_t)partial) << 32;
				}
			} else {
				// Меньше 4 байт - только часть первого uint32_t
				uint32_t partial = Data[idx];
				uint32_t mask = (1UL << (remaining_bytes * 8)) - 1;
				partial &= mask;
				current_data = (uint64_t)partial;
			}
		}

		// Выполняем запись
		HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, current_address, current_data);

		// Барьер памяти после записи
		__DMB();

		FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

		// Проверяем статус
		// if (status != HAL_OK) {
		//     __DMB();
		//     Lock();
		//     __set_PRIMASK(primask);
		//     return;
		// }
	}

	// Барьер памяти перед завершением
	__DMB();
	HAL_FLASH_Lock();

	__set_PRIMASK(primask);
}
*/

static inline void uint32_to_bytes_pointer(uint32_t value, uint8_t* bytes) {
	*((uint32_t*)bytes) = value;
}

static inline uint32_t bytes_to_uint32_pointer(const uint8_t* bytes) {
	return *((uint32_t*)bytes);
}

void Set_tx_s(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e) {
	tx_settings[0] = a;
	tx_settings[1] = b;
	tx_settings[2] = c;
	tx_settings[3] = d;
	tx_settings[4] = e;
}

void G4_echo() { // TODO derpecated
	// std::string str = "ships .. ";
	// std::string stat;
	// for (int i = 0; i < 5; ++i) {
	// 	rx_settings[i] = 0;
	// }
	// int x = 0;
	// for (auto n : numbers_chips) {  // TODO numbers_chips -> vChips
	// 	for (int ii = 0; ii < 3; ++ii) {
	// 		UART4_SendAddress(x);
	// 		Set_tx_s((uint8_t)bootloader_command::echo, 0, 0, 0, 0);
	// 		UART4_Send_Settings_bootloader();
	// 		rx_settings[0] = 0;
	// 		if (UART4_Receive_timeout_10us() && rx_settings[0]) { // TODO странно, ну ладно
	// 			str += " UART timeout " + std::to_string(n);
	// 		}
	// 		if (rx_settings[4] == 1) {
	// 			ship_is = state::bootloader;
	// 			stat = " bootloaders";
	// 		}
	// 		else {
	// 			ship_is = state::piano;
	// 			stat = " pianos";
	// 		}
	// 	}
	// 	if (rx_settings[0]) {
	// 		n = rx_settings[0];
	// 		str += std::format(" {}", n);
	// 	}
	// 	else {
	// 		n = 0;
	// 	}
	// 	if (rx_settings[1] != 0 || rx_settings[2] != 0 || rx_settings[3] != 0) { // проверка, что приняты " aadr 0 0 0 state"
	// 		str += "\n  * * NOISE!!! * *  ";
	// 	}
	// 	++x;
	// }
	// str += stat;
	// debugg_fn(str);
}

void data_from_H7_to_g4() {
	uint32_t start_adress_memory_read = ADRESS_H7_MAIN_FIRMWARE_FOR_G4; //  ++0x800 с каждым шагом, 6 копирований надо сделать
	uint32_t mem = ADRESS_G4_MAIN_FIRMWARE_ALT;
	std::string ships_ok = "ships flash ok .. ";
	int bug = 0;

	// g4 echo .... // TODO зачем, если уже собрал номера чипов?
	// G4_echo();
	// g4 echo .... // TODO зачем, если уже собрал номера чипов?
	init_chips(); // ?


	for (const auto& chip : vChips) { // TODO numbers_chips -> vChips ok
		start_adress_memory_read = ADRESS_H7_MAIN_FIRMWARE_FOR_G4;
		mem = ADRESS_G4_MAIN_FIRMWARE_ALT;

		for (uint32_t ii = 0; ii < COUNT_PAGE_FOR_FIRMWARE_G4; ++ii) {
			UART4_SendAddress(chip.number_chip);
			Set_tx_s((uint8_t)bootloader_command::data_from_H7_to_array_g4, 0x11, 0x12, 0x13, 0x14);
			UART4_Send_Settings_bootloader();

			// теперь внутри    From_H7_to_array_g4(); 
			// *  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  *
			UART4_Receive_Settings_bootloader(); // >> 0x11, 0x12, 0x13, 0x14
			// -   - -   - - -   - -   - - -   - -   - - -   - -   - - -   - -   - - -   - -   - - -   

			uint32_t primask = __get_PRIMASK();
			volatile uint32_t* pFlashAddr = (volatile uint32_t*)start_adress_memory_read;
			pause(1);
			for (uint32_t i = 0; i < 512; ++i) { // 2kB (4*512) размер пакета с прошивкой для отправки в g4
				uint32_to_bytes_pointer(pFlashAddr[i], tx_settings);
				tx_settings[4] = (uint8_t)i;
				pause(1);
				UART4_Send_Settings_bootloader();
				UART4_Receive_Settings_bootloader();
				if (bytes_to_uint32_pointer(rx_settings) != bytes_to_uint32_pointer(tx_settings)) {
					++bug;
				}
			}

			__set_PRIMASK(primask);
			UART4_Receive_Settings_bootloader(); // response::ok
			pause(2);

			// _+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+_
			// _+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+_
			// _+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+_

			flash_g4(mem, chip.number_chip);

			// _+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+_
			// _+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+_
			// _+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+_

			start_adress_memory_read += 0x800;
			mem += 0x800;
			pause(2);
		}
	// debugg_fn(ships_ok);
	}
	// прыгаем по предустановленному в G4 адресу (0x08008000)
	if (!bug) {
		debugg_fn("\n \n    JUMPING ");
	// 	for (auto n : numbers_chips) {
	// 		if (n) {
	// 			UART4_SendAddress(n);
	// 			Set_tx_s(command_for_flash_g4::jump_to_piano_g4, 0x03, 0x02, 0x01, 0x00);
	// 			UART4_Send_Settings_bootloader();
	// 			UART4_Receive_Settings_bootloader();
	// 			UART4_Receive_Settings_bootloader();
	// 			UART4_Receive_Settings_bootloader();
	// 			if (rx_settings[0] != n) {
	// 				debugg_fn(std::format(" start {} fail \n", n)); // DEBUG
	// 			}
	// 			pause(200);
	// 		}
	// 	}
		jump_g4s_to_adress();
	}
	else {
		debugg_fn(std::format("{} -- copy bugs", bug));
	}
}

void flash_g4(const uint32_t& addr, const int& chip_number) {


	UART4_SendAddress(chip_number);
	Set_tx_s((uint8_t)bootloader_command::copy_array_to_flash_g4, 0x61, 0x62, 0x63, 0x64);
	UART4_Send_Settings_bootloader();

	// теперь внутри From_array_g4_to_H7();
	UART4_Receive_Settings_bootloader(); // принимает ответ 0x15 0x61 0x62 0x63 0x64

	// **  ****  ****  ****  ****  ****  ****  ****  ****  **
	pause(2);

	// 1 отправка адреса
	uint32_to_bytes_pointer(addr, tx_settings);
	tx_settings[4] = chip_number; // просто так ..
	UART4_Send_Settings_bootloader();

	// 2 принять адрес для проверки
	UART4_Receive_Settings_bootloader();
	pause(1);
	uint32_t addr_back = bytes_to_uint32_pointer(rx_settings);
	if (addr_back == addr) {
		// debugg_fn(std::format("  addr  ok  {:x}", addr_back));
		Set_tx_s((uint8_t)response::ok, 0x45, 0x46, 0x47, 0x48);
	}
	else {
		debugg_fn(std::format("  addr  fail  {:x}", addr_back));
		Set_tx_s((uint8_t)response::fail, 0x55, 0x56, 0x57, 0x58);
	}

	// 3 если ок - то разрешаем запись
	UART4_Send_Settings_bootloader();

	//3.2
	UART4_Receive_Settings_bootloader(); // 32

	//3.5
	UART4_Receive_Settings_bootloader(); // 35

	// 4
	UART4_Receive_Settings_bootloader(); // fail or ok

	if (rx_settings[1] == (uint8_t)response::ok) {
		// debugg_fn(std::format("  FLASH G4 ok  {}", chip_number));
	}
	else {
		debugg_fn(std::format("  FLASH G4 fail  ((  {}", chip_number));
	}
}

void jump_g4s_to_adress() {
	int bug = 0;
	// G4_echo();
	init_chips();
	for (const auto& chip : vChips) { // TODO numbers_chips -> vChips ok

		UART4_SendAddress(chip.number_chip);
		Set_tx_s((uint8_t)bootloader_command::jump_g4_to_adress, 0x88, 0x88, 0x88, 0x88);
		UART4_Send_Settings_bootloader();
		UART4_Receive_Settings_bootloader();
		UART4_Receive_Settings_bootloader(); // принять (ChipN_32, 0x88, 0x88, 0x88, 0x88)

		if (bytes_to_uint32_pointer(&rx_settings[1]) != bytes_to_uint32_pointer(&tx_settings[1])) {
			++bug;
		}
		pause(4);

		// отправка адреса
		uint32_to_bytes_pointer(ADRESS_G4_MAIN_FIRMWARE_ALT, tx_settings);
		tx_settings[4] = chip.number_chip;
		UART4_Send_Settings_bootloader();
		UART4_Receive_Settings_bootloader(); // принять (полученный g4 адрес)
		uint32_t addr_back = bytes_to_uint32_pointer(rx_settings);
		pause(1);

		// если ок, то прыгаем
		if (addr_back == ADRESS_G4_MAIN_FIRMWARE_ALT) {
			Set_tx_s((uint8_t)response::ok, 0x03, 0x02, 0x01, 0x00);
			UART4_Send_Settings_bootloader();
		}
		else {
			debugg_fn(std::format("  jumping addr  bug  {:x} != {}", addr_back, ADRESS_G4_MAIN_FIRMWARE_ALT));
		}
		UART4_Receive_Settings_bootloader();
		UART4_Receive_Settings_bootloader();
		if (rx_settings[0] != chip.number_chip) {
			debugg_fn(std::format(" start {} fail \n", chip.number_chip)); // DEBUG
		}
		pause(200);
	}

	if (bug) {
		debugg_fn(std::format(" jump to address FAIL {} bugs", bug));
		return;
	}
	// pause(50000); // 50ms
}

void reset_bootloaders() { // TODO numbers_chips -> vChips ok
	if (chip_state == chip_states::boot) {
		for (const auto& chip : vChips) {
			UART4_SendAddress(chip.number_chip);
			Set_tx_s((uint8_t)bootloader_command::reset, 0x04, 0x03, 0x02, 0x01); // 200ms delay
			UART4_Send_Settings_bootloader();
			UART4_Receive_Settings_bootloader();
		}
	}
	else {
		debugg_fn(" state   NOT BOOTLOADER ");
	}
	pause(500000);
}

void reset_main_to_bootloader() {
	G4_echo();
	if (chip_state == chip_states::main) {
		for (const auto& chip : vChips) { // TODO numbers_chips -> vChips ok
			UART4_SendAddress(chip.number_chip);
			UART4_Send_Settings(command::reset_to_bootloader, 3, 2, 1);
		}
	}
	else {
		debugg_fn(" state   NOT MAIN ");
	}
}

const comparator& searcher_addr_in_cursor(const uint32_t& c) {
	std::map<uint8_t, comparator>* mComparatorCursor_temp;

	if (cur_disp == current_display::on) {
		mComparatorCursor_temp = &mComparatorCursor_on;
	}
	else {
		mComparatorCursor_temp = &mComparatorCursor_off;
	}

	auto it = mComparatorCursor_temp->find(c);

	if (it != mComparatorCursor_temp->end()) {
		return it->second;
	}
	else {
		return default_comparator; // initial value of reference to non-const must be an lvalueC/C++(461)
	}
}

// LVGL ACTIONS
#ifdef __cplusplus
extern "C" {

	// to dispays
	void action_to_main_disp(lv_event_t* e) {
		LL_USART_RequestRxDataFlush(UART5); // TODO это дожно быть здесь? (сбрасывает uart если какие-то данные предварительно были посланы из g4)
		pause(2);

		if (cur_disp == current_display::on) {
			for (const auto& chip : vChips) {
				if (chip.typ == typeAction::on) {
					sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::stop_calibration);
				}
				else {
					sender(command::unmute, chip.number_chip, 0, dot::green, 0);  // TODO chips mute_unmute
				}
			}
		}

		if (cur_disp == current_display::off) {
			for (const auto& chip : vChips) {
				if (chip.typ == typeAction::off) {
					sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::stop_calibration);
				}
				else {
					sender(command::unmute, chip.number_chip, 0, dot::green, 0);  // TODO chips mute_unmute
				}
			}

		}

		cur_disp = current_display::dis_main;
		loadScreen(SCREEN_ID_D_MAIN);
		debugg_clear();

		all_H7_to_g4();
		sync();

		// LL_USART_EnableDMAReq_RX(UART5); // это уже есть внутри sync();
		// LL_TIM_EnableCounter(TIM1);  // PWM - tim clk
	}

	void action_to_disp_calibration_on(lv_event_t* e) {
		LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
		LL_USART_DisableDMAReq_RX(UART5);
		cur_disp = current_display::on;
		cur_shart = objects.chart_on;
		loadScreen(SCREEN_ID_D_CHART_CALIB_ON);
		debugg_clear();
		all_g4_to_H7();
		cursor = vChips.front().comparators.front().cursor;
		lv_chart_set_cursor_point(objects.chart_on, cursor_on_vert, ser_on_blue, cursor);

		for (const auto& chip : vChips) {
			if (chip.typ == typeAction::on) {
				sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::start_calibration);
			}
			else {
				sender(command::mute, chip.number_chip, 0, dot::green, 0);  // TODO chips mute_unmute
			}
		}
	}

	void action_to_disp_calibration_off(lv_event_t* e) {
		LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
		LL_USART_DisableDMAReq_RX(UART5);
		cur_disp = current_display::off;
		cur_shart = objects.chart_off;
		loadScreen(SCREEN_ID_D_CHART_CALIB_OFF);
		debugg_clear();
		all_g4_to_H7();
		cursor = vChips.back().comparators.back().cursor;
		lv_chart_set_cursor_point(objects.chart_off, cursor_off_vert, ser_off_blue, cursor);

		for (const auto& chip : vChips) {
			if (chip.typ == typeAction::off) {
				sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::start_calibration);
			}
			else {
				sender(command::mute, chip.number_chip, 0, dot::green, 0);  // TODO chips mute_unmute
			}
		}
	}

	void action_to_disp_flash(lv_event_t* e) {
		LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
		LL_USART_DisableDMAReq_RX(UART5);
		cur_disp = current_display::d_flash;
		loadScreen(SCREEN_ID_D_FLASH);
	}

	// buttons
	void action__echo_g4s(lv_event_t* e) {
		// G4_echo(); // TODO deprecated
		init_chips();
	}

	void action_h7_g4(lv_event_t* e) { // копирует и шъёт одной кнопкой все чипы
		data_from_H7_to_g4();
	}

	void action_jump(lv_event_t* e) { // >> jump_to_piano_g4 по адресу // TODO ?
		jump_g4s_to_adress();
	}

	void action_reset_bootloader_g4(lv_event_t* e) { // TODO реализовать
		reset_bootloaders();
	}

	void action_reset_main_to_bootloader(lv_event_t* e) {
		reset_main_to_bootloader();
	}

		// other

	void action_to_disp_back(lv_event_t* e) {
		debugg_clear();
		if (cur_disp == current_display::on) {
			cur_shart = objects.d_chart_calib_on;
			lv_obj_set_parent(objects.chart_on, objects.d_chart_calib_on);
			loadScreen(SCREEN_ID_D_CHART_CALIB_ON);
		}
		else {
			cur_disp = current_display::off;
			cur_shart = objects.d_chart_calib_off;
			lv_obj_set_parent(objects.chart_off, objects.d_chart_calib_off);
			loadScreen(SCREEN_ID_D_CHART_CALIB_OFF);
		}
	}

	void action_calib_sensor_on_green(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor((uint32_t)cursor);
		sender(command::set_comp_value, comp.number_chip, comp.number_comparator, dot::green, buffer_calib[comp.address]);
		buffer_green[comp.address] = convert_8_16(a_, b_);
		sensor_on_1_data_string = std::to_string(buffer_green[comp.address]);
		lv_chart_refresh(cur_shart);
	}

	void action_calib_sensor_on_red(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor((uint32_t)cursor);
		sender(command::set_comp_value, comp.number_chip, comp.number_comparator, dot::red, buffer_calib[comp.address]);
		buffer_red[comp.address] = convert_8_16(a_, b_);
		sensor_on_2_data_string = std::to_string(buffer_red[comp.address]);
		lv_chart_refresh(cur_shart);
	}

	void action_calib_sensor_off_green(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor((uint32_t)cursor);
		sender(command::set_comp_value, comp.number_chip, comp.number_comparator, dot::green, buffer_calib[comp.address]);
		buffer_green[comp.address] = convert_8_16(a_, b_);
		sensor_off_1_data_string = std::to_string(buffer_green[comp.address]);
		lv_chart_refresh(cur_shart);
	}

	void action_calib_sensor_off_red(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor((uint32_t)cursor);
		sender(command::set_comp_value, comp.number_chip, comp.number_comparator, dot::red, buffer_calib[comp.address]);
		buffer_red[comp.address] = convert_8_16(a_, b_);
		sensor_off_2_data_string = std::to_string(buffer_red[comp.address]);
		lv_chart_refresh(cur_shart);
	}

	void set_cursor_piont_on(const comparator& comp) {
		cursor_string = std::to_string(cursor);
		lv_chart_set_cursor_point(objects.chart_on, cursor_on_vert, ser_on_green, cursor);
		sensor_on_1_data_string = std::to_string(buffer_green[comp.address]);
		sensor_on_2_data_string = std::to_string(buffer_red[comp.address]);

	}

	void set_cursor_piont_off(const comparator& comp) {
		cursor_string = std::to_string(cursor);
		lv_chart_set_cursor_point(objects.chart_off, cursor_off_vert, ser_off_green, cursor);
		sensor_off_1_data_string = std::to_string(buffer_green[comp.address]);
		sensor_off_2_data_string = std::to_string(buffer_red[comp.address]);

	}

	void action_cursor_minus(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor(cursor - 1);
		if (comp.is_active) {
			cursor -= 1;
			set_cursor_piont_on(comp);
		}
	}

	void action_cursor_plus(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor(cursor + 1);
		if (comp.is_active) {
			cursor += 1;
			set_cursor_piont_on(comp);
		}
	}

	void action_cursor_minus_7(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor(cursor - 7);
		if (comp.is_active) {
			cursor -= 7;
			set_cursor_piont_on(comp);
		}
	}

	void action_cursor_plus_7(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor(cursor + 7);
		if (comp.is_active) {
			cursor += 7;
			set_cursor_piont_on(comp);
		}
	}

	void action_save_calibration(lv_event_t* e) {
		SaveToMemory();
		debugg_fn(" calib saved "); // DEBUG
	}

	void action_restore_calibration(lv_event_t* e) {
		ReadOnMemory();
		LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
		LL_USART_DisableDMAReq_RX(UART5);
		all_H7_to_g4();
		LL_USART_EnableDMAReq_RX(UART5);
		LL_TIM_EnableCounter(TIM1);  // PWM - tim clk
		debugg_fn(" calib restored "); // DEBUG
	}



	void action_piano_off(lv_event_t* e) { // NVIC_SystemReset();
		SCB_DisableDCache();
		SCB_DisableICache();
		SCB_CleanInvalidateDCache();

		extern int* _bflag;
		uint32_t* dfu_boot_flag;
		dfu_boot_flag = (uint32_t*)(&_bflag);
		*dfu_boot_flag = 0xDEADBEEF;

		pause(200);
		NVIC_SystemReset();
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


	void action_add_1(lv_event_t* e) {
		chart_correction(1, plus_minus::plus);
	}

	void action_add_10(lv_event_t* e) {
		chart_correction(10, plus_minus::plus);
	}

	void action_add_100(lv_event_t* e) {
		chart_correction(100, plus_minus::plus);
	}

	void action_add_1000(lv_event_t* e) {
		chart_correction(1000, plus_minus::plus);
	}

	void action_sub_1(lv_event_t* e) {
		chart_correction(1, plus_minus::minus);
	}

	void action_sub_10(lv_event_t* e) {
		chart_correction(10, plus_minus::minus);
	}

	void action_sub_100(lv_event_t* e) {
		chart_correction(100, plus_minus::minus);
	}

	void action_sub_1000(lv_event_t* e) {
		chart_correction(1000, plus_minus::minus);
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
