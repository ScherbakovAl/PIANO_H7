/*
 * midi_keyboard_h7-2.cpp
 *
 *  Created on: Apr 8, 2025
 *      Author: sche
 */

#include "vector"
#include "map"
// #include <ranges>
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

	if (!__HAL_PWR_GET_FLAG(PWR_FLAG_SB)) {
		LCD_WR_REG(0x10); // Sleep In
		LL_mDelay(120);
		HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN4); //pin4 == кнопка К1 на плате
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
		HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4);
		HAL_PWR_EnterSTANDBYMode();
	}
	else {
		HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN4);
	}

}

void to_sleep() {

	// LCD_WR_REG(0x10); // Sleep In
	// LL_mDelay(120);
	// tud_disconnect();
	// HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN4);
	// __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
	// HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4);
	// HAL_PWR_EnterSTANDBYMode();
	LL_mDelay(200);
	NVIC_SystemReset();

}

void init() {

	init_LL();
	init_LCD_and_touch();
	lv_init();
	// pwr();
	disp_create_and_touch_start();
	ui_init();
	tusb_init();

	tud_task();
	lv_timer_handler();
	ui_tick();

	LL_USART_DisableDMAReq_RX(UART5);
	LL_TIM_DisableCounter(TIM1);  // PWM - tim clk

	init_buffers();
	init_midi_speeds();
	config_charts();
	init_chips();

	if (chip_state == chip_states::none) {
		debugg_fn("   *+*+*+*   NO CHIPS   *+*+*+*\n");
	}
	else if (chip_state == chip_states::piano) {
		debugg_fn("   *+*+*+*   piano   *+*+*+*\n");
		read_on_memory(); // восстановление графика при включении
		all_H7_to_g4();
		sync(); // включает прерывания и таймер, осторожно!
	}
	else if (chip_state == chip_states::boot1) {
		debugg_fn("   *+*+*+*   bootloader   *+*+*+*\n");
		jump_g4s_to_adress(ADDRESS_G4_MAIN_FIRMWARE, chip_states::piano);
		pause(50000);
		init_chips();
		read_on_memory(); // восстановление графика при включении
		all_H7_to_g4();
		sync(); // включает прерывания и таймер, осторожно!
	}
	else if (chip_state == chip_states::boot2) {
		debugg_fn("   *+*+*+*   bootloader v2   *+*+*+*\n");
	}
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

void init_LCD_and_touch() {

	// LCD init
	LL_SPI_Enable(SPI3);
	LL_SPI_StartMasterTransfer(SPI3);
	LCD_Init();

	// TOUCH init
	// LL_I2C_Enable(I2C5);
	FT6336_Init();

}

void disp_create_and_touch_start() {

	disp = lv_display_create(320, 480);
	lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB888);
	lv_display_set_flush_cb(disp, my_flush_cb);
	lv_display_set_buffers(disp, buf_1, buf_2, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

	LL_TIM_EnableIT_UPDATE(TIM6); // для TOUCH
	indev = lv_indev_create();
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, my_input_read);
}

void init_chips() {

	LL_USART_RequestRxDataFlush(UART5);

	vChips.clear();
	vChips.reserve(allChipCount);
	mCursor_to_comparator_on.clear();
	mCursor_to_comparator_off.clear();
	counter_on = 0;
	counter_off = 0;
	chip_state = chip_states::none;
	chip_state_prev = chip_states::none;
	int flag1 = 1;

	for (uint32_t i = 0; i < rx_settings_length; ++i) {
		rx_settings[i] = 0;
	}

	std::string strOut = "ships .. \n";
	std::string stat;


	for (uint8_t chip_N = 1; chip_N < allChipCount; ++chip_N) {
		rx_settings[0] = 0;
		UART4_send_address(chip_N);
		Set_tx_s((uint8_t)bootloader_command::echo, 0, 0, 0, 0);
		UART4_send_settings_bootloader();
		UART4_receive_timeout_10us();

		pause(4);
		UART4_send_address(chip_N); // с первого раза не раздупляются почему-то
		UART4_send_settings_bootloader();
		UART4_receive_timeout_10us();

		if (rx_settings[1] != 0 || rx_settings[2] != 0 || rx_settings[3] != 0) { // проверка, что приняты " aadr 0 0 0 state"
			strOut += "\n  * * NOISE!!! * *  ";
		}

		if (rx_settings[0]) {
			uint8_t addr_in_buffer = 0;
			std::vector<comparator> vComparators;
			vComparators.reserve(count_comparators);

			for (uint8_t comp_N = 0; comp_N < count_comparators; ++comp_N) {

				if (chip_N < on_off_division) {
					addr_in_buffer = (chip_N * count_comparators) + comp_N;
					mCursor_to_comparator_on.emplace(counter_on, comparator(counter_on, addr_in_buffer, chip_N, comp_N, true));
					vComparators.push_back(comparator(counter_on, addr_in_buffer, chip_N, comp_N, true));
					++counter_on;
				}
				else {
					addr_in_buffer = ((chip_N - on_off_division) * count_comparators) + comp_N + buffer_division;
					mCursor_to_comparator_off.emplace(counter_off, comparator(counter_off, addr_in_buffer, chip_N, comp_N, true));
					vComparators.push_back(comparator(counter_off, addr_in_buffer, chip_N, comp_N, true));
					++counter_off;
				}
			}
			vChips.push_back({ chip_N, vComparators, (chip_N < on_off_division ? typeAction::on : typeAction::off), chip_state });

			chip_state = (chip_states)rx_settings[4];

			if (flag1) {
				chip_state_prev = chip_state;
				flag1 = 0;
			}

			if (chip_state != chip_state_prev) {
				chip_state_prev = chip_state;
				strOut += std::format("  chip_state fail {} ", chip_N);
			}

			strOut += std::format(" {}", chip_N);
		}
	}

	if (chip_state == chip_states::boot1) {
		stat = "\n bootloaders";
	}
	else if (chip_state == chip_states::piano) {
		stat = "\n pianos";
	}
	else if (chip_state == chip_states::none) {
		stat.clear();
		strOut.clear();
	}
	else if (chip_state == chip_states::boot2) {
		stat = "\n bootloaders v2";
	}

	strOut += std::format(" = {}", (counter_on + counter_off) / count_comparators);
	strOut += stat;
	debugg_fn(strOut);
}

void init_buffers() {

	for (uint32_t i = 0; i < buffer_division; ++i) {
		buffer_green[i] = def_comp.green_on_default;
		buffer_red[i] = def_comp.red_on_default;
		buffer_blue_calib[i] = 0;
		buffer_blue_calib_old[i] = 0;
		buffer_dac[i] = 0;
	}
	for (uint32_t i = buffer_division; i < size_BUFFER; ++i) {
		buffer_green[i] = def_comp.green_off_default;
		buffer_red[i] = def_comp.red_off_default;
		buffer_blue_calib[i] = 0;
		buffer_blue_calib_old[i] = 0;
		buffer_dac[i] = 0;
	}

}

void init_midi_speeds() {

	for (uint i = 0; i < size_BUFFER; ++i) {
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

	// for (uint32_t i = 0; i < size_BUFFER; ++i) {
	// 	mass_F[i] = key_mass; //  + (float)i / 10000000; // 8 гр
	// }

	curve_OFF.push_back(speed_for_midi(8000.0f, 1710.0f, 45000.0f, -0.44f)); // расстояние 1710 
	curve_OFF.push_back(speed_for_midi(80000.0f, 1710.0f, 25000.0f, -3.0f)); // << это используется в noteOFF
	curve_OFF.push_back(speed_for_midi(800000.0f, 1100.0f, 85000.0f, -17.8f));


	curve_ON.push_back(speed_for_midi(0.0f, 0.0f, 0.0f, 0.0f)); // ---
	curve_ON.push_back(speed_for_midi(58005.0f, 1710.0f, 18250.0f, -1.14f));	// -normal
	curve_ON.push_back(speed_for_midi(600000.0f, 1710.0f, 75300.0f, -12.5f)); 	// -bright
	curve_ON.push_back(speed_for_midi(800.0f, 1710.0f, -600.0f, 1.0f)); 		// -muffled
	curve_ON.push_back(speed_for_midi(98005.0f, 1710.0f, 28250.0f, -2.3f)); 	// -experiment
	curve_ON.push_back(speed_for_midi(120000.0f, 1710.0f, 33000.0f, -3.0f)); 	// -new
	curve_ON.push_back(speed_for_midi(600000.0f, 1710.0f, 75500.0f, -13.0f)); 	// -new 2
	curve_ON.push_back(speed_for_midi(200000.0f, 1710.0f, 43000.0f, -6.0f)); 	// -new 3

	// const float key_mass = 89.0f;
	// const float interval = 1710.0f;

	// // const float mass_bass = 120000.0f;
	// // const float mass_discant = 50000.0f;
	// const float mass_bass = 58005.0f;
	// const float mass_discant = 58000.0f;
	// const float mass_step = (mass_bass - mass_discant) / key_mass;

	// // const float x_bass = 31775.0f; // v1
	// // const float x_discant = 19250.0f; // v1
	// // const float y_start = -1.18f; // v1
	// // const float y_fin = -0.53f; // v1

	// // const float x_bass = 28000.0f; // v2
	// // const float x_discant = 28000.0f; // v2
	// // const float y_start = -4.5f; // v2
	// // const float y_fin = -2.1f; // v2

	// // const float x_bass = 31425.0f; // v3
	// // const float x_discant = 18625.0f; // v3
	// // const float y_start = -3.78f; // v3
	// // const float y_fin = -1.88f; // v3

	// const float x_bass = 18251.0f; // v4
	// const float x_discant = 18250.0f; // v4
	// const float y_start = -1.18f; // v4
	// const float y_fin = -1.17f; // v4

	// const float x_step = (x_bass - x_discant) / key_mass;
	// const float y_step = (y_start - y_fin) / key_mass;

	// float m_s = mass_bass;
	// float x_s = x_bass;
	// float y_s = y_start;

	// for (int i = 0; i < 98; ++i) {
	// 	speeds_ON.push_back(speed_for_midi(m_s, interval, x_s, y_s));
	// 	m_s -= mass_step;
	// 	x_s -= x_step;
	// 	y_s -= y_step;
	// }
}

void config_charts() {
	lv_obj_t* ob = objects.chart_on;
	// lv_chart_set_point_count(ob, buffer_division);
	lv_chart_set_point_count(ob, 91);
	ser_on_green = lv_chart_add_series(ob, lv_color_hex(0x00ff00), LV_CHART_AXIS_PRIMARY_X);
	ser_on_red = lv_chart_add_series(ob, lv_color_hex(0xff0000), LV_CHART_AXIS_PRIMARY_X);
	ser_on_blue = lv_chart_add_series(ob, lv_color_hex(0x0dcaf3), LV_CHART_AXIS_PRIMARY_X); // LV_COLOR_MAKE(0xE9, 0x1E, 0x63)
	ser_on_dac = lv_chart_add_series(ob, lv_color_hex(0x757575), LV_CHART_AXIS_PRIMARY_X);
	lv_chart_set_series_ext_y_array(ob, ser_on_green, &buffer_green[7]);
	lv_chart_set_series_ext_y_array(ob, ser_on_red, &buffer_red[7]);
	lv_chart_set_series_ext_y_array(ob, ser_on_blue, &buffer_blue_calib[7]);
	lv_chart_set_series_ext_y_array(ob, ser_on_dac, &buffer_dac[7]);
	lv_chart_set_axis_range(ob, LV_CHART_AXIS_PRIMARY_Y, on_green_max, on_green_min);
	lv_chart_set_axis_range(ob, LV_CHART_AXIS_SECONDARY_Y, on_red_max, on_red_min);
	cursor_on_vert = lv_chart_add_cursor(ob, lv_color_hex(0x808080), LV_DIR_VER);
	cursor_on_hor = lv_chart_add_cursor(ob, lv_color_hex(0x808080), LV_DIR_HOR);
	lv_obj_set_style_line_width(ob, 1, LV_PART_CURSOR); // толщина курсора
	lv_obj_set_style_line_width(ob, 0, LV_PART_ITEMS);  // толщина линий между точками на графике
	lv_obj_set_style_size(ob, 2, 3, LV_PART_INDICATOR); // размер точек на графике
	lv_chart_set_div_line_count(ob, 0, 0);
	lv_obj_set_style_radius(ob, 0, 0);

	ob = objects.chart_off;
	lv_chart_set_point_count(ob, 70);
	ser_off_green = lv_chart_add_series(ob, lv_color_hex(0x00ff00), LV_CHART_AXIS_PRIMARY_X);
	ser_off_red = lv_chart_add_series(ob, lv_color_hex(0xff0000), LV_CHART_AXIS_PRIMARY_X);
	ser_off_blue = lv_chart_add_series(ob, lv_color_hex(0x0dcaf3), LV_CHART_AXIS_PRIMARY_X);
	ser_off_dac = lv_chart_add_series(ob, lv_color_hex(0x757575), LV_CHART_AXIS_PRIMARY_X);
	lv_chart_set_series_ext_y_array(ob, ser_off_green, &buffer_green[buffer_division]);
	lv_chart_set_series_ext_y_array(ob, ser_off_red, &buffer_red[buffer_division]);
	lv_chart_set_series_ext_y_array(ob, ser_off_blue, &buffer_blue_calib[buffer_division]);
	lv_chart_set_series_ext_y_array(ob, ser_off_dac, &buffer_dac[buffer_division]);
	lv_chart_set_axis_range(ob, LV_CHART_AXIS_PRIMARY_Y, off_green_max, off_green_min);
	lv_chart_set_axis_range(ob, LV_CHART_AXIS_SECONDARY_Y, off_red_max, off_red_min);
	cursor_off_vert = lv_chart_add_cursor(ob, lv_color_hex(0x808080), LV_DIR_VER); // lv_color_make(200, 200, 200)
	cursor_off_hor = lv_chart_add_cursor(ob, lv_color_hex(0x808080), LV_DIR_HOR);
	lv_obj_set_style_line_width(ob, 1, LV_PART_CURSOR); // толщина курсора
	lv_obj_set_style_line_width(ob, 0, LV_PART_ITEMS);  // толщина линий между точками на графике
	lv_obj_set_style_size(ob, 2, 3, LV_PART_INDICATOR); // размер точек на графике
	lv_chart_set_div_line_count(ob, 0, 0);
	lv_obj_set_style_radius(ob, 0, 0);
}

void reconfig_charts() { // должен быть только после config_charts();s

	// uint8_t start_on = 0;
	// uint8_t end_on = 0;
	// uint8_t start_off = 0;
	// uint8_t end_off = 0;

	// if (!vChips.empty()) {

	// 	const auto& front = vChips.front();
	// 	const auto& back = vChips.back();

	// 	if (front.typ == typeAction::on) {
	// 		start_on = front.comparators.front().address;
	// 	}
	// 	else {
	// 		start_off = front.comparators.front().address;
	// 	}

	// 	if (back.typ == typeAction::off) {
	// 		end_off = back.comparators.back().address;
	// 	}
	// 	else {
	// 		end_on = back.comparators.back().address;
	// 	}

	// 	if (front.typ == typeAction::on && back.typ == typeAction::off) {
	// 		for (const auto& [prev, curr] : vChips | std::views::adjacent<2>) {
	// 			if (curr.typ == typeAction::off) {
	// 				end_on = prev.comparators.back().address;
	// 				start_off = curr.comparators.front().address;
	// 				return;
	// 			}
	// 		}
	// 	}

	// 	if (counter_on) {
	// 		lv_chart_set_point_count(objects.chart_on, end_on - start_on);
	// 		lv_chart_set_series_ext_y_array(objects.chart_on, ser_on_green, &buffer_green[start_on]);
	// 		lv_chart_set_series_ext_y_array(objects.chart_on, ser_on_red, &buffer_red[start_on]);
	// 		lv_chart_set_series_ext_y_array(objects.chart_on, ser_on_blue, &buffer_blue_calib[start_on]);
	// 		lv_chart_set_series_ext_y_array(objects.chart_on, ser_on_dac, &buffer_dac[start_on]);
	// 	}
	// 	if (counter_off) {
	// 		lv_chart_set_point_count(objects.chart_off, end_off - start_off);
	// 		lv_chart_set_series_ext_y_array(objects.chart_off, ser_off_green, &buffer_green[start_off]);
	// 		lv_chart_set_series_ext_y_array(objects.chart_off, ser_off_red, &buffer_red[start_off]);
	// 		lv_chart_set_series_ext_y_array(objects.chart_off, ser_off_blue, &buffer_blue_calib[start_off]);
	// 		lv_chart_set_series_ext_y_array(objects.chart_off, ser_off_dac, &buffer_dac[start_off]);
	// 	}

	// 	debugg_fn(std::format("cur {}", start_on)); // DEBUG
	// }
}

// добавить анимацию: https://duino.ru/blog/onlayn-konverter-gif-animatsii-v-iskhodnyy-kod-dlya-arduino/
void h7() {

	init();

	while (1) { // основной цикл

		tud_task();
		lv_timer_handler_run_in_period(5);
		ui_tick();

		if (TIM5->CNT > 3000) { // 1000 = 1ms (чтобы калибровка не наступала себе на пятки)

			if (cur_disp == current_display::on) {

				// for (const auto& [prev, current] : vChips | std::views::slide(2)) {}
				// for (auto n : v | std::views::drop(3) | std::views::take(4)) {}
				for (const auto& chip : vChips) {
					if (chip.typ == typeAction::on) {
						sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::read_calibration);
						for (const auto& comp : chip.comparators) {
							UART4_receive_settings();
							buffer_blue_calib[comp.address] = convert_8_16(a_, b_);
						}
						update_cursor(chip);
					}
				}
				uint8_t addr = mCursor_to_comparator_on[cursor].address;
				chart_calib_online = std::to_string(buffer_blue_calib[addr]);
				l = std::to_string(buffer_blue_calib[addr - 1]);
				r = std::to_string(buffer_blue_calib[addr + 1]);
				lv_chart_set_cursor_point(objects.chart_on, cursor_on_hor, ser_on_blue, cursor);
				lv_chart_refresh(cur_chart);
			}

			if (cur_disp == current_display::off) {

				for (const auto& chip : vChips) {
					if (chip.typ == typeAction::off) {
						sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::read_calibration);
						for (const auto& comp : chip.comparators) {
							UART4_receive_settings();
							buffer_blue_calib[comp.address] = convert_8_16(a_, b_);
						}
						update_cursor(chip);
					}
				}
				uint8_t addr = mCursor_to_comparator_off[cursor].address;
				chart_calib_online = std::to_string(buffer_blue_calib[addr]);
				l = std::to_string(buffer_blue_calib[addr - 1]);
				r = std::to_string(buffer_blue_calib[addr + 1]);
				lv_chart_set_cursor_point(objects.chart_off, cursor_off_hor, ser_off_blue, cursor);
				lv_chart_refresh(cur_chart);
			}
			TIM5->CNT = 0;
		}

		if (fl == 1) { // DEBUG

			// tud_disconnect();
			// debugg_fn("usb disconnect!")
			debugg_fn(std::format("min {},    max {}", min, max));
			// debugg_fn(std::format("  .   .    .   . . "));
			// debugg_fn(std::format("tx = {:#04x} - {:#04x} - {:#04x} - {:#04x} - {:#04x}", tx_settings[0], tx_settings[1], tx_settings[2], tx_settings[3], tx_settings[4]));
			// debugg_fn(std::format("rx = {:#04x} - {:#04x} - {:#04x} - {:#04x} - {:#04x}", rx_settings[0], rx_settings[1], rx_settings[2], rx_settings[3], rx_settings[4]));
			// debugg_fn(std::format("tOut = { :.5f }", rx_settings[1]));
			// debugg_fn(std::format("rx_settings = {:}", rx_settings[1]));
			// test_t_out_fl = std::format("{:.10f}", timerLenght_F); // DEBUG
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
			fl = 0; // DEBUG
		}

		if (fl == 2) {
			debugg_fn(std::format(" OVER {}  m{:.2f}  d{}  x{}  y{:.2f} ", out_debug_, m_F, sd_F, sx_F, sy_F));
			fl = 0;
		}

		if (fl == 3) {
			debugg_fn(std::format("  {}", speed_F));
			fl = 0;
		}
	}
} // h7

void sync() { // включает прерывания, осторожно!
	LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
	LL_USART_DisableDMAReq_RX(UART5);

	TIM3->CNT = 0; // сбросить номер контроллера
	int fl_sync = 0;
	for (const auto& chip : vChips) {
		fl_sync += sync_sender(chip.number_chip);
	}
	if (fl_sync) {
		debugg_fn(std::format("Sync {} bugs", fl_sync));
	}
	else {
		debugg_fn("   sync OK");
	}

	LL_USART_EnableDMAReq_RX(UART5);
	LL_TIM_EnableCounter(TIM1);  // PWM - tim clk
}

int sync_sender(const uint8_t& i) {
	int fs = 0;
	UART4_send_address(i);
	pause(3);
	UART4_send_settings(command::sync_timer, 0, 0, 0);
	// pause(1);
	// UART4_Receive_Settings();
	if (UART4_receive_timeout_10us()) {
		debugg_fn("UART timeout " + std::to_string(i));
	}
	if (b_ != 0 && a_ != 0 && rx_settings[0] != i) {
		debugg_fn("Sync err, mcu  #" + std::to_string(i));
		++fs;
	}
	pause(1);
	return fs;
}

void all_H7_to_g4() {
	pause(10); // если вдруг кто-то захочет что-то отправить... ?

	if (!vChips.empty()) {
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
	debugg_fn("   calib H7 >> G4 \n");
}

void all_g4_to_H7() {
	pause(10); // если вдруг кто-то захочет что-то отправить... ?

	if (!vChips.empty()) {
		for (const auto& chip : vChips) {
			for (const auto& comp : chip.comparators) {
				sender(command::read_comp_value, chip.number_chip, comp.number_comparator, dot::green, 0);
				buffer_green[comp.address] = convert_8_16(a_, b_);
				sender(command::read_comp_value, chip.number_chip, comp.number_comparator, dot::red, 0);
				buffer_red[comp.address] = convert_8_16(a_, b_);
			}
		}
	}
	debugg_fn("   calib G4 >> H7 \n");
}

void read_comp_setting(const typeAction& t) {
	pause(10);

	if (!vChips.empty()) {
		for (const auto& chip : vChips) {
			if (chip.typ == t) {
				for (const auto& comp : chip.comparators) {
					sender(command::read_comp_setting, chip.number_chip, comp.number_comparator, dot::green, 0);
					buffer_dac[comp.address] = convert_8_16(a_, b_);
				}
			}
		}
	}
}

void update_cursor(const Chip& chip) {
	bool fl_c = false;
	for (const auto& comp : chip.comparators) {
		const auto& addr = comp.address;
		if (buffer_blue_calib[addr] < 4080 && buffer_blue_calib[addr] > 2) {

			if (buffer_blue_calib_old[addr] + 200 < buffer_blue_calib[addr]) {
				buffer_blue_calib_old[addr] = buffer_blue_calib[addr];
				fl_c = true;
			}
			else if (buffer_blue_calib_old[addr] - 200 > buffer_blue_calib[addr]) {
				buffer_blue_calib_old[addr] = buffer_blue_calib[addr];
				fl_c = true;
			}
		}

		if (fl_c) {
			cursor = comp.cursor;
			if (addr > buffer_division) {
				lv_chart_set_cursor_point(objects.chart_off, cursor_off_vert, ser_off_blue, cursor);
				sensor_off_1_data_string = std::to_string(buffer_green[addr]);
				sensor_off_2_data_string = std::to_string(buffer_red[addr]);
				cursor_string = std::to_string(cursor);
			}
			else {
				lv_chart_set_cursor_point(objects.chart_on, cursor_on_vert, ser_on_blue, cursor);
				sensor_on_1_data_string = std::to_string(buffer_green[addr]);
				sensor_on_2_data_string = std::to_string(buffer_red[addr]);
				cursor_string = std::to_string(cursor);
			}
			n_chip = std::to_string(comp.number_chip);
			n_comp = std::to_string(comp.number_comparator);
			fl_c = false;
		}
	}
}

void sender(const command& com, const uint8_t& adress, const uint8_t& compN, const dot& dot, const uint32_t& value) {
	UART4_send_address(adress);
	pause(3); // 4 for release, 10-debug g4
	UART4_send_settings(com, compN, (uint8_t)dot, value);
	UART4_receive_settings();
	pause(1); // 1 for release, 4-debug g4
	if (adress != rx_settings[0]) {
		debugg_fn(std::format("BAD ADRESS {}   rx_settings {}", adress, rx_settings[0]));
	}
	if (com == command::set_comp_value) {
		if (convert_8_16(a_, b_) != value) {
			debugg_fn(std::format("BAD SET DATA rx={}, add={}, comp={}, dot={}, value={}, in={}", rx_settings[0], adress, compN, (uint8_t)dot, value, convert_8_16(a_, b_)));
		}
	}
	if (com == command::mute || com == command::unmute) {
		if (convert_8_16(a_, b_) != (int32_t)chip_states::piano) {
			debugg_fn(std::format(" mute - unmute  fail {}", adress));
		}
	}
}

// UART Send-Recive
void UART4_send_address(const uint8_t& slave_address) {
	const uint16_t address_byte = slave_address | 0x100; // Установка старшего бита (MSB) для указания адреса
	while (!LL_USART_IsActiveFlag_TXE(UART5)) {}
	LL_USART_TransmitData9(UART5, address_byte);
	while (!LL_USART_IsActiveFlag_TC(UART5)) {}
	pause(2);
}

void UART4_send_settings(const command& com, const uint8_t& compN, const uint8_t& dot, const uint32_t& value) {
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

void UART4_receive_settings() {
	for (uint8_t i = 0; i < rx_settings_length; i++) {
		while (!LL_USART_IsActiveFlag_RXNE(UART5)) {}
		rx_settings[i] = (uint8_t)LL_USART_ReceiveData9(UART5);
	}
	compN_ = rx_settings[1];
	dot_ = rx_settings[2];
	a_ = rx_settings[3];
	b_ = rx_settings[4];
}

void UART4_send_settings_bootloader() { // tx_settings[0] - [4]
	while (!LL_USART_IsActiveFlag_TXE(UART5)) {}
	for (uint16_t i = 0; i < tx_settings_length; i++) {
		LL_USART_TransmitData9(UART5, tx_settings[i]);
		while (!LL_USART_IsActiveFlag_TXE(UART5)) {}
	}
	while (!LL_USART_IsActiveFlag_TC(UART5)) {}
}

void UART4_receive_settings_bootloader() { // rx_settings[0] - [4]
	for (uint8_t i = 0; i < rx_settings_length; i++) {
		while (!LL_USART_IsActiveFlag_RXNE(UART5)) {}
		rx_settings[i] = (uint8_t)LL_USART_ReceiveData9(UART5);
	}
}

int UART4_receive_timeout_10us() {
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

// DMA IQR Handler
void DMA1_RX(void) {


	LL_DMA_ClearFlag_TC2(DMA1);
	LL_TIM_DisableCounter(TIM1); // PWM - tim clk


	if (LL_USART_IsActiveFlag_NE(UART5)) {
		LL_USART_ClearFlag_NE(UART5);
		USART_noise_error_detected();
		debugg_fn("...noise");
	}
	else {

		SCB_CleanInvalidateDCache();

		const uint32_t rxB = rx_data[0];

		// const uint32_t tim_compare = TIM3->CNT;
		// if (tim_compare != rxB / count_comparators) { // TODO сделать проверку номера чипа!
		// 	debugg_fn(std::format(" chip number err {} != {}", rxB, tim_compare));
		// }

		midi_hi_F = 0.0f;
		midi_lo_F = 0.0f;
		// timer_data_in = 0;


		if (rxB > 168) { // DEBUG
			USART_noise_error_detected();
			debugg_fn("... rxB > 168");
		}

		uint32_t tOut = 0;
		tOut = rx_data[1] << 16 | rx_data[2] << 8 | rx_data[3];
		float integerPart_F;
		uint8_t note_ = rxB + noteAdder[rxB];


		if (rxB > 98) { // for OFF
			const speed_for_midi& st = curve_OFF[1]; // скорость: 0 - глухая ... 2 - яркая ? (выше я сделал три варианта скорости off на выбор)
			speed_F = st.distance / ((float)tOut + st.offset_x);
			energy_F = ((st.key_mass * speed_F * speed_F) / 2.0f) + st.offset_y;
			midi_hi_F = energy_F;
			midi_lo_F = modf(midi_hi_F, &integerPart_F) * maxMidi_F;
		}
		else { // for ON

			const uint32_t& curve_num = lv_roller_get_selected(objects.roller);

			if (curve_num) {

				const speed_for_midi& curve = curve_ON[curve_num];

				speed_F = curve.distance / (((float)tOut) + curve.offset_x);
				energy_F = ((curve.key_mass * speed_F * speed_F) / 2.0f) + curve.offset_y;
				midi_hi_F = energy_F;
				midi_lo_F = modf(midi_hi_F, &integerPart_F) * maxMidi_F;

			}
			else { // curve_num > 0, v1 старые значения, для использования вместе с программной-rust кривой на компе

				const float key_mass = 0.008f; // 8 гр -->> переехал в массив
				const float distance_F = 0.0017f; // 1.7 мм (толщина шаблонов 1.9 и 0.2)
				const float div_on = 0.000000000092f; // меньше - громче
				const float deriv_F = 2.0f; // делить на 2 в формуле
				const float maxMidi_F = 127.99f;
				float timerLenght_F = (float)tOut * div_on;

				speed_F = distance_F / timerLenght_F;
				energy_F = (key_mass * speed_F * speed_F) / deriv_F;
				midi_hi_F = energy_F / maxMidi_F;
				float integerPart_F;
				midi_lo_F = modf(midi_hi_F, &integerPart_F) * maxMidi_F;
				int note_ = rxB + noteAdder[rxB];

			}

		}

		if (midi_hi_F < 1.0f) {
			midi_hi_F = 1.0f;
			midi_lo_F = 1.0f;
		}
		if (midi_hi_F > 127.0f) {
			midi_hi_F = 127.0f;
			midi_lo_F = 127.0f;
			// out_debug_ = tOut;
			// m_F = 58005; // DEBUG
			// sd_F = 1710;
			// sx_F = 18250;
			// sy_F = -1.18;
			// fl = 2;
		}

		uint8_t note_buf[] = {
			0xB0,
			0x58,
			(uint8_t)midi_lo_F,
			(uint8_t)rxB < 98U ? 0x90U : 0x80U, // 0x90 note on
			note_,
			(uint8_t)midi_hi_F
		};

		tud_midi_stream_write(0, note_buf, 6);

		if (rxB > 76 && rxB < 98) { // верхние ноты без демпферов
			uint8_t note_buff[] = {
				0xB0,
				0x58,
				(uint8_t)midi_lo_F,
				0x80,
				note_,
				(uint8_t)midi_hi_F
			};
			tud_midi_stream_write(0, note_buff, 6);
		}

		if (rxB < 98) { // DEBUG
			if (tOut > max) {
				max = tOut;
				fl = 1;
			}
			if (tOut < min) {
				min = tOut;
				fl = 1;
			}
		}

		// speed_F = tOut; // DEBUG
		// timerLenght_F = midi_hi_F; // DEBUG
		// speed_F = midi_hi_F; // DEBUG
		// fl = rxB < 98 ? 3 : 0; // DEBUG // разрешить обновлять цифры на дисплее
	}

	LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_2);
	LL_TIM_EnableCounter(TIM1); // PWM - tim clk

}

void USART_noise_error_detected() {
	debugg_fn(std::format("USART Noise Error detected {}-{}-{}-{}", rx_data[0], rx_data[1], rx_data[2], rx_data[3]));
	LL_USART_RequestRxDataFlush(UART5);
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

int32_t convert_8_16(const uint8_t& a, const uint8_t& b) {
	int32_t x = a << 8 | b;
	return x;
}

conv_16_8 convert_16_8(const uint32_t& a) {
	conv_16_8 result;
	// TODO вариант: r.a = (a & (0xff << 8)) >> 8; или r.a = (a >> 8) & 0xff;
	result.a = (a & 0xff << 8) >> 8;
	result.b = a & 0xff;
	return result;
}

void save_to_memory() {

	SCB_DisableICache();
	SCB_DisableDCache();
	HAL_FLASH_Unlock();

	FLASH_Erase_Sector(FLASH_SECTOR_7, FLASH_BANK_1, FLASH_VOLTAGE_RANGE_2);

	uint32_t Addr = ADDRESS_H7_CALIB;
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

void read_on_memory() {
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
		buffer_green[i] = *(volatile uint32_t*)(ADDRESS_H7_CALIB + (i * sizeof(uint32_t)));
		buffer_red[i] = *(volatile uint32_t*)(ADDRESS_H7_CALIB + offset_buffer_red + (i * sizeof(uint32_t)));
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

void debugg_fn(const std::string& str) {
	if (debug_counter % 13 == 0)debugg_clear();
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

// LVGL UTILITES
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

void Read_uint32(uint32_t Address, volatile uint32_t* pData, uint32_t Size) {

	//  * @brief Чтение данных из Flash памяти в формате uint32_t
	//  * @param Address: Адрес начала чтения во Flash (должен быть выровнен по 4 байта)
	//  * @param pData: Указатель на массив uint32_t для сохранения прочитанных данных
	//  * @param Size: Размер данных в БАЙТАХ (не в элементах uint32_t!)
	//  * @note  Функция безопасна для вызова из прерывания
	//  * @note  Размер Size будет округлен вверх до кратного 4
	//  *
	//  * Пример использования:
	//  * __attribute__((aligned(32))) volatile uint32_t bin_data_32[512];
	//  * Read_uint32(0x08008000, bin_data_32, 2048); // Прочитать 2048 байт (512 uint32_t)

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

void data_from_H7_to_g4(const uint32_t& adress_g4_firmware_in_H7, const uint32_t& count_page, const uint32_t& addr_jump) {

	uint32_t start_adress_memory_read = adress_g4_firmware_in_H7; //  ++0x800 с каждым шагом
	uint32_t mem = addr_jump;
	std::string ships_ok = "ships flash ok .. ";
	int bug = 0;

	init_chips();

	for (const auto& chip : vChips) {
		start_adress_memory_read = adress_g4_firmware_in_H7;
		mem = addr_jump;

		for (uint32_t ii = 0; ii < count_page; ++ii) {
			UART4_send_address(chip.number_chip);
			Set_tx_s((uint8_t)bootloader_command::data_from_H7_to_array_g4, 0x11, 0x12, 0x13, 0x14);
			UART4_send_settings_bootloader();

			// теперь внутри    From_H7_to_array_g4(); 
			// *  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  *
			UART4_receive_settings_bootloader(); // >> 0x11, 0x12, 0x13, 0x14
			// -   - -   - - -   - -   - - -   - -   - - -   - -   - - -   - -   - - -   - -   - - -   

			uint32_t primask = __get_PRIMASK();
			volatile uint32_t* pFlashAddr = (volatile uint32_t*)start_adress_memory_read;
			pause(1);
			for (uint32_t i = 0; i < 512; ++i) { // 2kB (4*512) размер пакета с прошивкой для отправки в g4
				uint32_to_bytes_pointer(pFlashAddr[i], tx_settings);
				tx_settings[4] = (uint8_t)i;
				pause(1);
				UART4_send_settings_bootloader();
				UART4_receive_settings_bootloader();
				if (bytes_to_uint32_pointer(rx_settings) != bytes_to_uint32_pointer(tx_settings)) {
					++bug;
				}
			}

			__set_PRIMASK(primask);
			UART4_receive_settings_bootloader(); // response::ok
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
	if (bug) {
		debugg_fn(std::format("{} -- copy bugs", bug));
	}
	else {
		// debugg_clear();
		// debugg_fn("\n \n    JUMPING ");
		// jump_g4s_to_adress(addr_jump);

		/*
		for (auto n : numbers_chips) {
			if (n) {
				UART4_SendAddress(n);
				Set_tx_s(command_for_flash_g4::jump_to_piano_g4, 0x03, 0x02, 0x01, 0x00);
				UART4_Send_Settings_bootloader();
				UART4_Receive_Settings_bootloader();
				UART4_Receive_Settings_bootloader();
				UART4_Receive_Settings_bootloader();
				if (rx_settings[0] != n) {
					debugg_fn(std::format(" start {} fail \n", n));
				}
				pause(200);
			}
		}
		*/
	}
}

void flash_g4(const uint32_t& addr, const int& chip_number) {


	UART4_send_address(chip_number);
	Set_tx_s((uint8_t)bootloader_command::copy_array_to_flash_g4, 0x61, 0x62, 0x63, 0x64);
	UART4_send_settings_bootloader();

	// теперь внутри From_array_g4_to_H7();
	UART4_receive_settings_bootloader(); // принимает ответ 0x15 0x61 0x62 0x63 0x64

	// **  ****  ****  ****  ****  ****  ****  ****  ****  **
	pause(2);

	// 1 отправка адреса
	uint32_to_bytes_pointer(addr, tx_settings);
	tx_settings[4] = chip_number; // просто так ..
	UART4_send_settings_bootloader();

	// 2 принять адрес для проверки
	UART4_receive_settings_bootloader();
	pause(1);
	uint32_t addr_back = bytes_to_uint32_pointer(rx_settings);
	if (addr_back == addr) {
		Set_tx_s((uint8_t)response::ok, 0x45, 0x46, 0x47, 0x48);
	}
	else {
		debugg_fn(std::format("  addr  fail  {:x}", addr_back));
		Set_tx_s((uint8_t)response::fail, 0x55, 0x56, 0x57, 0x58);
	}

	// 3 если ок - то разрешаем запись
	UART4_send_settings_bootloader();

	//3.2
	UART4_receive_settings_bootloader(); // 32

	//3.5
	UART4_receive_settings_bootloader(); // 35

	// 4
	UART4_receive_settings_bootloader(); // fail or ok

	if (rx_settings[1] == (uint8_t)response::ok) {
	}
	else {
		debugg_fn(std::format("  FLASH G4 fail  ((  {}", chip_number));
	}
}

void jump_g4s_to_adress(const uint32_t& addr_jump, const chip_states& jump_to_) {

	init_chips();
	int bug = 0;

	for (const auto& chip : vChips) {

		UART4_send_address(chip.number_chip);
		Set_tx_s((uint8_t)bootloader_command::jump_g4_to_adress, 0x88, 0x88, 0x88, 0x88);
		UART4_send_settings_bootloader();
		UART4_receive_settings_bootloader();
		UART4_receive_settings_bootloader(); // принять (ChipN_32, 0x88, 0x88, 0x88, 0x88)

		if (bytes_to_uint32_pointer(&rx_settings[1]) != bytes_to_uint32_pointer(&tx_settings[1])) {
			++bug;
		}
		pause(4);

		// отправка адреса
		uint32_to_bytes_pointer(addr_jump, tx_settings);
		tx_settings[4] = chip.number_chip;
		UART4_send_settings_bootloader();
		UART4_receive_settings_bootloader(); // принять (полученный g4 адрес)
		uint32_t addr_back = bytes_to_uint32_pointer(rx_settings);
		pause(1);

		// если ок, то прыгаем
		if (addr_back == addr_jump) {
			Set_tx_s((uint8_t)response::ok, 0x03, 0x02, 0x01, 0x00);
			UART4_send_settings_bootloader();
		}
		else {
			debugg_fn(std::format("  jumping addr  bug  {:x} != {}", addr_back, addr_jump));
		}
		UART4_receive_settings_bootloader();

		if (jump_to_ == chip_states::boot2) {
			rx_settings[1] = 1;
			while (rx_settings[1] != 0) {
				UART4_send_address(chip.number_chip);
				Set_tx_s((uint8_t)bootloader_command::echo, 0x88, 0x88, 0x88, 0x88);
				UART4_send_settings_bootloader();
				UART4_receive_timeout_10us();
			}
		}
		else if (jump_to_ == chip_states::boot1) {
			rx_settings[1] = 1;
			while (rx_settings[1] != 0) {
				UART4_send_address(chip.number_chip);
				Set_tx_s((uint8_t)bootloader_command::echo, 0x88, 0x88, 0x88, 0x88);
				UART4_send_settings_bootloader();
				UART4_receive_timeout_10us();
			}
		}
		else if (jump_to_ == chip_states::piano) {
			UART4_receive_settings_bootloader();
		}

		if (rx_settings[0] != chip.number_chip) {
			debugg_fn(std::format(" start {} fail \n", chip.number_chip));
		}

		pause(2);
	}

	if (bug) {
		debugg_fn(std::format(" jump to address FAIL {} bugs", bug));
		return;
	}
}

void reset_bootloaders() {

	if (chip_state == chip_states::boot1) {
		for (const auto& chip : vChips) {
			UART4_send_address(chip.number_chip);
			Set_tx_s((uint8_t)bootloader_command::reset, 0x04, 0x03, 0x02, 0x01); // 200ms delay
			UART4_send_settings_bootloader();
			UART4_receive_settings_bootloader();
			rx_settings[4] = 0;
			while (rx_settings[4] != 1) {
				UART4_send_address(chip.number_chip);
				Set_tx_s((uint8_t)bootloader_command::echo, 0x88, 0x88, 0x88, 0x88);
				UART4_send_settings_bootloader();
				UART4_receive_timeout_10us();
			}
		}
	}
	else if (chip_state == chip_states::boot2) {
		for (const auto& chip : vChips) {
			UART4_send_address(chip.number_chip);
			Set_tx_s((uint8_t)bootloader_command::reset, 0x04, 0x03, 0x02, 0x01); // 200ms delay
			UART4_send_settings_bootloader();
			UART4_receive_settings_bootloader();
			rx_settings[4] = 0;
			while (rx_settings[4] != 1) {
				UART4_send_address(chip.number_chip);
				Set_tx_s((uint8_t)bootloader_command::echo, 0x88, 0x88, 0x88, 0x88);
				UART4_send_settings_bootloader();
				UART4_receive_timeout_10us();
			}
		}
	}
	else {
		debugg_fn(" state   NOT BOOTLOADER ");
	}
	pause(500000);
}

void reset_main_to_bootloader() {

	init_chips();
	if (chip_state == chip_states::piano) {
		for (const auto& chip : vChips) {
			UART4_send_address(chip.number_chip);
			UART4_send_settings(command::reset_to_bootloader, 3, 2, 1);
			rx_settings[4] = 0;
			while (rx_settings[4] != 1) {
				UART4_send_address(chip.number_chip);
				Set_tx_s((uint8_t)bootloader_command::echo, 0x88, 0x88, 0x88, 0x88);
				UART4_send_settings_bootloader();
				UART4_receive_timeout_10us();
			}
		}
	}
	else {
		debugg_fn(" state   NOT MAIN ");
	}
}

const comparator& searcher_addr_in_cursor(const uint32_t& c) {
	std::map<uint8_t, comparator>* mComparatorCursor_temp;

	if (cur_disp == current_display::on) {
		mComparatorCursor_temp = &mCursor_to_comparator_on;
	}
	else {
		mComparatorCursor_temp = &mCursor_to_comparator_off;
	}

	auto it = mComparatorCursor_temp->find(c);

	if (it != mComparatorCursor_temp->end()) {
		return it->second;
	}
	else {
		return default_comparator;
	}
}

void set_cursor_piont_on(const comparator& comp) {
	cursor_string = std::to_string(cursor);
	lv_chart_set_cursor_point(objects.chart_on, cursor_on_vert, ser_on_blue, cursor);
	sensor_on_1_data_string = std::to_string(buffer_green[comp.address]);
	sensor_on_2_data_string = std::to_string(buffer_red[comp.address]);
	n_chip = std::to_string(comp.number_chip);
	n_comp = std::to_string(comp.number_comparator);
}

void set_cursor_piont_off(const comparator& comp) {
	cursor_string = std::to_string(cursor);
	lv_chart_set_cursor_point(objects.chart_off, cursor_off_vert, ser_off_blue, cursor);
	sensor_off_1_data_string = std::to_string(buffer_green[comp.address]);
	sensor_off_2_data_string = std::to_string(buffer_red[comp.address]);
	n_chip = std::to_string(comp.number_chip);
	n_comp = std::to_string(comp.number_comparator);
}

// LVGL ACTIONS
#ifdef __cplusplus
extern "C" {

	// to dispays
	void action_to_main_disp(lv_event_t* e) {
		LL_USART_RequestRxDataFlush(UART5); // TODO это дожно быть здесь? (сбрасывает uart если какие-то данные предварительно были посланы из g4)
		pause(2);

		if (!vChips.empty()) {
			if (cur_disp == current_display::on) {
				for (const auto& chip : vChips) {
					if (chip.typ == typeAction::on) {
						sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::stop_calibration);
					}
					else if (chip.typ == typeAction::off) {
						sender(command::unmute, chip.number_chip, 0, dot::green, 0);
					}
				}
			}

			if (cur_disp == current_display::off) {
				for (const auto& chip : vChips) {
					if (chip.typ == typeAction::off) {
						sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::stop_calibration);
					}
					else if (chip.typ == typeAction::on) {
						sender(command::unmute, chip.number_chip, 0, dot::green, 0);
					}
				}

			}
		}

		cur_disp = current_display::main;
		loadScreen(SCREEN_ID_D_MAIN);
		debugg_clear();

		if (chip_state == chip_states::piano) {
			all_H7_to_g4();
			sync();
		}
	}

	void action_to_disp_calibration_on(lv_event_t* e) {
		LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
		LL_USART_DisableDMAReq_RX(UART5);
		cur_disp = current_display::on;
		cur_chart = objects.chart_on;
		loadScreen(SCREEN_ID_D_CHART_CALIB_ON);
		debugg_clear();
		all_g4_to_H7();
		if (!vChips.empty()) {
			cursor = vChips.front().comparators.front().cursor;
			lv_chart_set_cursor_point(objects.chart_on, cursor_on_vert, ser_on_blue, cursor);
			read_comp_setting(typeAction::on);

			for (const auto& chip : vChips) {
				if (chip.typ == typeAction::on) {
					sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::start_calibration);
				}
				else if (chip.typ == typeAction::off) {
					sender(command::mute, chip.number_chip, 0, dot::green, 0);
				}
			}
		}
	}

	void action_to_disp_calibration_off(lv_event_t* e) {
		LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
		LL_USART_DisableDMAReq_RX(UART5);
		cur_disp = current_display::off;
		cur_chart = objects.chart_off;
		loadScreen(SCREEN_ID_D_CHART_CALIB_OFF);
		debugg_clear();
		if (!vChips.empty()) {
			all_g4_to_H7();
			cursor = vChips.back().comparators.back().cursor;
			lv_chart_set_cursor_point(objects.chart_off, cursor_off_vert, ser_off_blue, cursor);
			read_comp_setting(typeAction::off);

			for (const auto& chip : vChips) {
				if (chip.typ == typeAction::off) {
					sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::start_calibration);
				}
				else if (chip.typ == typeAction::on) {
					sender(command::mute, chip.number_chip, 0, dot::green, 0);
				}
			}
		}
	}

	void action_to_disp_flash(lv_event_t* e) {
		LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
		LL_USART_DisableDMAReq_RX(UART5);
		cur_disp = current_display::bootloader;
		loadScreen(SCREEN_ID_D_FLASH);
	}

	// buttons
	void action__echo_g4s(lv_event_t* e) {
		init_chips();
	}

	void action_h7_g4(lv_event_t* e) {
		data_from_H7_to_g4(ADDRESS_H7_MAIN_FIRMWARE_FOR_G4, COUNT_PAGE_FOR_FIRMWARE_G4, ADDRESS_G4_MAIN_FIRMWARE);
	}

	void action_jump(lv_event_t* e) {
		jump_g4s_to_adress(ADDRESS_G4_MAIN_FIRMWARE, chip_states::piano);
	}

	void action_boot_1_flash_boot_2(lv_event_t* e) {
		data_from_H7_to_g4(ADDRESS_H7_MAIN_FIRMWARE_FOR_G4, COUNT_PAGE_G4_FOR_BOOTLOADER_v2, ADDRESS_G4_BOOTLOADER_v2);
	}

	void action_in_boot_jump_to_boot_2(lv_event_t* e) {
		jump_g4s_to_adress(ADDRESS_G4_BOOTLOADER_v2, chip_states::boot2);
	}

	void action_boot_2_flash_boot_1(lv_event_t* e) {
		data_from_H7_to_g4(ADDRESS_H7_MAIN_FIRMWARE_FOR_G4, COUNT_PAGE_G4_FOR_BOOTLOADER, ADDRESS_G4_BOOTLOADER);
	}

	void action_reset_bootloader_g4(lv_event_t* e) {
		reset_bootloaders();
	}

	void action_reset_main_to_bootloader(lv_event_t* e) {
		reset_main_to_bootloader();
	}

	void action_calib_sensor_on_green(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor((uint32_t)cursor);
		sender(command::set_comp_value, comp.number_chip, comp.number_comparator, dot::green, buffer_blue_calib[comp.address]);
		buffer_green[comp.address] = convert_8_16(a_, b_);
		sensor_on_1_data_string = std::to_string(buffer_green[comp.address]);
	}

	void action_calib_sensor_on_red(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor((uint32_t)cursor);
		sender(command::set_comp_value, comp.number_chip, comp.number_comparator, dot::red, buffer_blue_calib[comp.address]);
		buffer_red[comp.address] = convert_8_16(a_, b_);
		sensor_on_2_data_string = std::to_string(buffer_red[comp.address]);
	}

	void action_calib_sensor_off_green(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor((uint32_t)cursor);
		sender(command::set_comp_value, comp.number_chip, comp.number_comparator, dot::green, buffer_blue_calib[comp.address]);
		buffer_green[comp.address] = convert_8_16(a_, b_);
		sensor_off_1_data_string = std::to_string(buffer_green[comp.address]);
	}

	void action_calib_sensor_off_red(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor((uint32_t)cursor);
		sender(command::set_comp_value, comp.number_chip, comp.number_comparator, dot::red, buffer_blue_calib[comp.address]);
		buffer_red[comp.address] = convert_8_16(a_, b_);
		sensor_off_2_data_string = std::to_string(buffer_red[comp.address]);
	}

	void action_cursor_minus(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor(cursor - 1);
		if (comp.is_active) {
			cursor -= 1;
			set_cursor_piont_on(comp);
			set_cursor_piont_off(comp);
		}
	}

	void action_cursor_plus(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor(cursor + 1);
		if (comp.is_active) {
			cursor += 1;
			set_cursor_piont_on(comp);
			set_cursor_piont_off(comp);
		}
	}

	void action_cursor_minus_7(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor(cursor - 7);
		if (comp.is_active) {
			cursor -= 7;
			set_cursor_piont_on(comp);
			set_cursor_piont_off(comp);
		}
	}

	void action_cursor_plus_7(lv_event_t* e) {
		const auto& comp = searcher_addr_in_cursor(cursor + 7);
		if (comp.is_active) {
			cursor += 7;
			set_cursor_piont_on(comp);
			set_cursor_piont_off(comp);
		}
	}

	void action_save_calibration(lv_event_t* e) {
		save_to_memory();
		debugg_fn(" calib saved "); // DEBUG
	}

	void action_restore_calibration(lv_event_t* e) {
		read_on_memory();
		LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
		LL_USART_DisableDMAReq_RX(UART5);
		all_H7_to_g4();
		LL_USART_EnableDMAReq_RX(UART5);
		LL_TIM_EnableCounter(TIM1);  // PWM - tim clk
		debugg_fn(" calib restored "); // DEBUG
	}

	void action_jump_to_dfu(lv_event_t* e) {
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

	void action_piano_off(lv_event_t* e) {
		to_sleep();
	}

	void action_clear_disp(lv_event_t* e) {
		debugg_clear();
	}

		// LVGL VARS
	const char* get_var_debugg() {
		return debugg.c_str();
	}
	void set_var_debugg(const char* value) {
		debugg = value;
	}

	const char* get_var_cursor_string() {
		return cursor_string.c_str();
	}
	void set_var_cursor_string(const char* value) {
		cursor_string = value;
	}

	const char* get_var_chart_calib_online() {
		return chart_calib_online.c_str();
	}
	void set_var_chart_calib_online(const char* value) {
		chart_calib_online = value;
	}

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

	const char* get_var_n_chip() {
		return n_chip.c_str();
	}
	void set_var_n_chip(const char* value) {
		n_chip = value;
	}

	const char* get_var_n_comp() {
		return n_comp.c_str();
	}
	void set_var_n_comp(const char* value) {
		n_comp = value;
	}

}
#endif // extern "C"
