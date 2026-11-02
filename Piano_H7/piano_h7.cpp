/*
 * midi_keyboard_h7-2.cpp
 *
 *  Created on: Apr 8, 2025
 *      Author: sche
 */

#include "vector"
#include "map"
// #include <ranges>
#include "string"
#include "algorithm"
#include "piano_h7.hpp"

// Состояние; что значит каждая переменная — в piano_h7.hpp
alignas(32) uint8_t rx_data[32] = { };
__attribute__((aligned(8))) uint8_t tx_settings[5] = { };
__attribute__((aligned(8))) uint8_t rx_settings[5] = { };
uint8_t compN_ = 0;
uint8_t dot_ = 0;
uint8_t a_ = 0;
uint8_t b_ = 0;

int32_t buffer_green[size_BUFFER] = {};
int32_t buffer_red[size_BUFFER] = {};
int32_t buffer_blue_calib[size_BUFFER] = {};
int32_t buffer_blue_calib_old[size_BUFFER] = {};
int32_t buffer_dac[size_BUFFER] = {};
struct default_comp def_comp;
default_graph_min_max graph_min_max;

std::vector<Chip> vChips;
std::map<uint8_t, comparator> mCursor_to_comparator_on;
std::map<uint8_t, comparator> mCursor_to_comparator_off;
static comparator default_comparator; // для вывода из searcher_addr_in_cursor() когда нет объекта для возврата по ссылке

chip_states chip_state = chip_states::none;
chip_states chip_state_prev = chip_states::none;
current_display cur_disp = current_display::none;
uint32_t cursor = 0;
bool calib_follow = true;
uint32_t calib_dirty = 0;
bool calib_data_new = false;

int8_t noteAdder[size_BUFFER] = {};

int counter_on = 0;
int counter_off = 0;

bool chip_silent[allChipCount] = { };
bool chip_bad_adress[allChipCount] = { };
bool chip_bad_data[allChipCount] = { };

bool send_damper_lift = true;

volatile uint32_t noise_count = 0;
uint32_t noise_shown = 0;
volatile uint8_t noise_bytes[4] = { };
volatile uint32_t midi_lost = 0;
uint32_t midi_lost_shown = 0;
volatile uint32_t g4_ring[g4_ring_size] = { };
volatile uint32_t g4_ring_w = 0;
volatile uint32_t g4_ring_r = 0;
volatile uint32_t g4_ring_lost = 0;
uint32_t g4_ring_lost_shown = 0;
bool host_reads = false;
uint32_t host_tx_done_seen = 0;
uint32_t host_tx_edge_ms = 0;
volatile uint32_t lcd_dma_errors = 0;
uint32_t lcd_dma_errors_shown = 0;
uint32_t touch_errors_shown = 0;
uint32_t touch_errors_tick = 0;
volatile uint8_t last_note = 0;
volatile uint8_t last_note_vel = 0;
volatile uint32_t last_note_t_us = 0;
volatile uint32_t last_note_seq = 0;

int fl = 0.0f;
float speed_F = 0.0f;
float midi_hi_F = 0.0f;
float midi_lo_F = 0.0f;
uint32_t out_debug_ = 0;
float timer_data_in = 0.0f;
uint32_t debug_counter = 0.0f;

float min = 391.2f;
float max = 19207.0f;

std::vector<speed_for_midi> curve_ON;
volatile uint8_t curve_on_selected = 9;
volatile uint8_t curve_off_selected = 0;
volatile uint8_t curve_lift_selected = 0;
user_curve_rec user_curve = { user_curve_magic, 461.0f, 18305.0f, 2188.0f, {} };
volatile float user_K = 0.0f;
volatile float user_y0 = 0.0f;
volatile float user_x0 = 0.0f;
volatile float user_lin_t_fast = vel_t_fast;
volatile float user_lin_t_slow = vel_t_slow;
volatile float user_log_t_fast = vel_t_fast;
volatile float user_log_t_slow = vel_t_slow;
float m_F = 0.0f;
float sd_F = 0.0f;
float sx_F = 0.0f;
float sy_F = 0.0f;

std::string debugg;

// inline constexpr uint32_t NOTE_OFFSETS[196] = {
//     // Индексы для коррекции MIDI номера
//     // Эти значения зависят от конкретной клавиатуры
//     [0 ... 53] = 14,    // 0-54: +14
//     [54 ... 95] = 13,   // 55-95: +13
//     [96 ... 145] = -77, // 96-145: -77
//     [146 ... 195] = -78 // 146+: -78
// };


void to_sleep() {

	LL_mDelay(200);
	NVIC_SystemReset();

}

void init() {

	init_LL();
	if (const user_curve_rec* saved = user_curve_saved()) {
		user_curve = *saved;
	}
	user_curve_set();
	gui_init();
	// TinyUSB живёт в PendSV (usb_task): приоритет как у OTG_HS — друг друга не прерывают; DMA1_Stream2 (0) выше обоих
	NVIC_SetPriority(PendSV_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 0));
	// В tinyusb/ есть свои правки (искать «PIANO_H7»): tud_midi_n_tx_available, _tx_done, _tx_clear, _tx_probe
	// и счётчик в midid_xfer_cb — class/midi/midi_device.c/.h. При обновлении библиотеки — перенести, иначе не соберётся
	tusb_init();

	lv_timer_handler();
	ui_tick();

	LL_USART_DisableDMAReq_RX(UART5);
	LL_TIM_DisableCounter(TIM1);  // PWM - tim clk

	init_buffers();
	init_midi_speeds();
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
			chip_state = (chip_states)rx_settings[4]; // до push_back: st — состояние этого чипа, а не предыдущего

			vChips.push_back({ chip_N, vComparators, (chip_N < on_off_division ? typeAction::on : typeAction::off), chip_state });

			if (flag1) {
				chip_state_prev = chip_state;
				flag1 = 0;
			}

			if (chip_state != chip_state_prev) {
				chip_state_prev = chip_state;
				strOut += "  chip_state fail " + std::to_string(chip_N) + " ";
			}

			strOut += " " + std::to_string(chip_N);
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

	strOut += " = " + std::to_string((counter_on + counter_off) / count_comparators);
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

	for (uint32_t i = 0; i < size_BUFFER; ++i) {
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


	// Времена в мкс (offset_x), distance в мкм — скорость в м/с. Пересчёт из прежних тиков 102.9 нс (k = 14/136):
	// offset_x · k, key_mass · k², distance как была — velocity та же.
	curve_ON.push_back(speed_for_midi(0.0f, 0.0f, 0.0f, 0.0f)); // ---
	curve_ON.push_back(speed_for_midi(614.672f, 1710.0f, 1878.68f, -1.14f));	// -normal
	curve_ON.push_back(speed_for_midi(6358.13f, 1710.0f, 7751.47f, -12.5f)); 	// -bright
	curve_ON.push_back(speed_for_midi(8.47751f, 1710.0f, -61.7647f, 1.0f)); 		// -muffled
	curve_ON.push_back(speed_for_midi(1038.55f, 1710.0f, 2908.09f, -2.3f)); 	// -experiment
	curve_ON.push_back(speed_for_midi(1271.63f, 1710.0f, 3397.06f, -3.0f)); 	// -new
	curve_ON.push_back(speed_for_midi(6358.13f, 1710.0f, 7772.06f, -13.0f)); 	// -new 2
	curve_ON.push_back(speed_for_midi(2119.38f, 1710.0f, 4426.47f, -6.0f)); 	// -new 3
	curve_ON.push_back(speed_for_midi(614.672f, 1710.0f, 2187.5f, -1.14f)); 	// -normal upd -10db D38db

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

		lv_timer_handler_run_in_period(5);
		ui_tick();

		if (TIM5->CNT > 3000) { // 1000 = 1ms (чтобы калибровка не наступала себе на пятки)
			if (cur_disp == current_display::on || cur_disp == current_display::off) {
				calib_poll();
			}
			TIM5->CNT = 0;
		}

		gui_tick();

		if (noise_count != noise_shown) {
			noise_shown = noise_count;
			debugg_printf("USART noise %" LV_PRIu32 " : %u-%u-%u-%u", noise_shown, (unsigned)noise_bytes[0], (unsigned)noise_bytes[1], (unsigned)noise_bytes[2], (unsigned)noise_bytes[3]);
		}

		if (midi_lost != midi_lost_shown) {
			midi_lost_shown = midi_lost;
			debugg_printf("MIDI lost %" LV_PRIu32, midi_lost_shown);
		}

		if (g4_ring_lost != g4_ring_lost_shown) {
			g4_ring_lost_shown = g4_ring_lost;
			debugg_printf("G4 ring lost %" LV_PRIu32, g4_ring_lost_shown);
		}

		if (lcd_dma_errors != lcd_dma_errors_shown) {
			lcd_dma_errors_shown = lcd_dma_errors;
			debugg_printf("LCD DMA error %" LV_PRIu32, lcd_dma_errors_shown);
			lv_obj_invalidate(lv_screen_active()); // кусок кадра не дошёл — перерисовать экран
		}

		// не чаще раза в секунду: отвалившийся тач ошибается на каждом опросе LVGL
		if (ft6336_errors != touch_errors_shown && HAL_GetTick() - touch_errors_tick >= 1000) {
			static const char* const names[] = { "OK", "BUSY", "NACK", "BERR/ARLO", "TIMEOUT" };
			touch_errors_shown = ft6336_errors;
			touch_errors_tick = HAL_GetTick();
			debugg_printf("Touch I2C error %" LV_PRIu32 ": %s", touch_errors_shown, names[ft6336_last_error]);
		}

		if (fl == 1) { // DEBUG

			// tud_disconnect();
			// debugg_fn("usb disconnect!")
			debugg_printf("min %d,    max %d", (int)min, (int)max);
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
			debugg_printf(" OVER %" LV_PRIu32 "  m%d  d%d  x%d  y%d ", out_debug_, (int)m_F, (int)sd_F, (int)sx_F, (int)sy_F);
			fl = 0;
		}

		if (fl == 3) {
			debugg_printf("  %d", (int)speed_F);
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
		debugg_printf("Sync %d bugs", fl_sync);
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
					sender(command::set_comp_value, chip.number_chip, comp.number_comparator, dot::grey, buffer_green[comp.address] - (buffer_green[comp.address] / 40));
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
				if (sender(command::read_comp_value, chip.number_chip, comp.number_comparator, dot::green, 0)) {
					buffer_green[comp.address] = convert_8_16(a_, b_);
				}
				if (sender(command::read_comp_value, chip.number_chip, comp.number_comparator, dot::red, 0)) {
					buffer_red[comp.address] = convert_8_16(a_, b_);
				}
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
					if (sender(command::read_comp_setting, chip.number_chip, comp.number_comparator, dot::green, 0)) {
						buffer_dac[comp.address] = convert_8_16(a_, b_);
					}
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
			if (calib_follow) {
				cursor = comp.cursor;
			}
			fl_c = false;
		}
	}
}

// Экраны калибровки. Шина стоит (TIM1 и DMA приёма выключены): G4 калибруемого типа гоняют ЦАП, другой — молчит
void calib_enter(const typeAction t) {
	LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
	LL_USART_DisableDMAReq_RX(UART5);
	cur_disp = t == typeAction::on ? current_display::on : current_display::off;
	if (vChips.empty()) {
		return;
	}
	all_g4_to_H7();
	cursor = t == typeAction::on ? vChips.front().comparators.front().cursor : vChips.back().comparators.back().cursor;
	read_comp_setting(t);

	for (const auto& chip : vChips) {
		if (chip.typ == t) {
			sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::start_calibration);
		}
		else {
			sender(command::mute, chip.number_chip, 0, dot::green, 0);
		}
	}
}

// Выход из калибровки без запуска шины: G4 калибруемого типа — stop_calibration, другим — unmute
void calib_leave() {
	LL_USART_RequestRxDataFlush(UART5); // TODO это дожно быть здесь? (сбрасывает uart если какие-то данные предварительно были посланы из g4)
	pause(2);

	if (!vChips.empty() && (cur_disp == current_display::on || cur_disp == current_display::off)) {
		const typeAction t = cur_disp == current_display::on ? typeAction::on : typeAction::off;
		for (const auto& chip : vChips) {
			if (chip.typ == t) {
				sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::stop_calibration);
			}
			else {
				sender(command::unmute, chip.number_chip, 0, dot::green, 0);
			}
		}
	}
	cur_disp = current_display::none;
}

// раз в 3 мс из основного цикла: синие значения калибруемых G4
void calib_poll() {
	const typeAction t = cur_disp == current_display::on ? typeAction::on : typeAction::off;
	for (const auto& chip : vChips) {
		if (chip.typ == t) {
			if (sender(command::all_calib, chip.number_chip, 0, dot::green, (uint32_t)subcommand::read_calibration)) {
				for (const auto& comp : chip.comparators) {
					if (UART4_receive_settings()) break;
					buffer_blue_calib[comp.address] = convert_8_16(a_, b_);
				}
				update_cursor(chip);
			}
		}
	}
	calib_data_new = true;
}

// порог d клавиши под курсором — текущее синее значение
void calib_set_threshold(const dot d) {
	const auto& comp = searcher_addr_in_cursor(cursor);
	if (!comp.is_active) {
		return;
	}
	sender(command::set_comp_value, comp.number_chip, comp.number_comparator, d, buffer_blue_calib[comp.address]);
	(d == dot::red ? buffer_red : buffer_green)[comp.address] = convert_8_16(a_, b_);
	++calib_dirty;
}

// курсор на delta клавиш, если там есть компаратор
void calib_cursor_move(const int32_t delta) {
	const auto& comp = searcher_addr_in_cursor(cursor + delta);
	if (comp.is_active) {
		cursor += delta;
	}
}

void calib_save() {
	save_to_memory();
	calib_dirty = 0;
	debugg_fn(" calib saved "); // DEBUG
}

void calib_restore() {
	read_on_memory();
	LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
	LL_USART_DisableDMAReq_RX(UART5);
	all_H7_to_g4();
	LL_USART_EnableDMAReq_RX(UART5);
	LL_TIM_EnableCounter(TIM1);  // PWM - tim clk
	calib_dirty = 0;
	debugg_fn(" calib restored "); // DEBUG
}

// экран прошивки G4: шина стоит
void bus_stop() {
	LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
	LL_USART_DisableDMAReq_RX(UART5);
	cur_disp = current_display::bootloader;
}

// с экранов калибровки и прошивки — обратно к игре
void bus_resume() {
	calib_leave();
	cur_disp = current_display::main;
	if (chip_state == chip_states::piano) {
		all_H7_to_g4();
		sync();
	}
}

// Пересканировать G4 без перезагрузки H7. Пока идёт игра, шину на время опроса останавливает;
// пороги — из RAM (несохранённые правки тоже), как после выхода из калибровки
void chips_rescan() {
	const bool playing = cur_disp != current_display::bootloader;
	if (playing) {
		LL_TIM_DisableCounter(TIM1);  // PWM - tim clk
		LL_USART_DisableDMAReq_RX(UART5);
	}
	init_chips();
	if (playing && chip_state == chip_states::piano) {
		all_H7_to_g4();
		sync();
	}
}

bool sender(const command& com, const uint8_t& adress, const uint8_t& compN, const dot& dot, const uint32_t& value) { // false — G4 не ответил
	UART4_send_address(adress);
	pause(3); // 4 for release, 10-debug g4
	UART4_send_settings(com, compN, (uint8_t)dot, value);
	if (UART4_receive_settings()) {
		if (!chip_silent[adress]) { // печатаем один раз, а не на каждом опросе калибровки
			chip_silent[adress] = true;
			debugg_printf("G4 #%d NO ANSWER (com %d)", adress, (int)com);
		}
		return false;
	}
	if (chip_silent[adress]) {
		chip_silent[adress] = false;
		debugg_printf("G4 #%d answers again", adress);
	}
	pause(1); // 1 for release, 4-debug g4
	const bool bad_adress = adress != rx_settings[0];
	if (bad_adress && !chip_bad_adress[adress]) { // один раз, пока G4 не ответит со своим адресом
		debugg_printf("BAD ADRESS %d   rx_settings %d", adress, rx_settings[0]);
	}
	chip_bad_adress[adress] = bad_adress;
	if (com == command::set_comp_value) {
		const bool bad_data = convert_8_16(a_, b_) != value;
		if (bad_data && !chip_bad_data[adress]) { // один раз, пока set_comp_value не пройдёт
			debugg_printf("BAD SET DATA rx=%d, add=%d, comp=%d, dot=%d, value=%" LV_PRIu32 ", in=%" LV_PRId32, rx_settings[0], adress, compN, (uint8_t)dot, value, convert_8_16(a_, b_));
		}
		chip_bad_data[adress] = bad_data;
	}
	if (com == command::mute || com == command::unmute) {
		if (convert_8_16(a_, b_) != (int32_t)chip_states::piano) {
			debugg_printf(" mute - unmute  fail %d", adress);
		}
	}
	return true;
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

int UART4_receive_settings() { // 1 — тайм-аут: G4 не ответил, не висим на шине
	const uint32_t timeout = 50000 * 275; // 50 мс (TIM2 275 МГц): ответ на stop_calibration ждёт конца прохода калибровки G4
	const uint32_t start = TIM2->CNT;
	for (uint8_t i = 0; i < rx_settings_length; i++) {
		while (!LL_USART_IsActiveFlag_RXNE(UART5)) {
			if (TIM2->CNT - start > timeout) {
				LL_USART_RequestRxDataFlush(UART5); // недопринятое — не в следующий ответ
				LL_USART_ClearFlag_ORE(UART5); // пока ORE стоит, приёмник байты выбрасывает — и DMA шины тоже
				return 1;
			}
		}
		rx_settings[i] = (uint8_t)LL_USART_ReceiveData9(UART5);
	}
	compN_ = rx_settings[1];
	dot_ = rx_settings[2];
	a_ = rx_settings[3];
	b_ = rx_settings[4];
	return 0;
}

void UART4_send_settings_bootloader() { // tx_settings[0] - [4]
	while (!LL_USART_IsActiveFlag_TXE(UART5)) {}
	for (uint16_t i = 0; i < tx_settings_length; i++) {
		LL_USART_TransmitData9(UART5, tx_settings[i]);
		while (!LL_USART_IsActiveFlag_TXE(UART5)) {}
	}
	while (!LL_USART_IsActiveFlag_TC(UART5)) {}
}

int UART4_receive_settings_bootloader() { // rx_settings[0] - [4]; 1 — тайм-аут: G4 не ответил, не висим на шине
	const uint32_t timeout = 200000 * 275; // 200 мс (TIM2 275 МГц): с запасом на стирание и запись страницы G4
	const uint32_t start = TIM2->CNT;
	for (uint8_t i = 0; i < rx_settings_length; i++) {
		while (!LL_USART_IsActiveFlag_RXNE(UART5)) {
			if (TIM2->CNT - start > timeout) {
				LL_USART_RequestRxDataFlush(UART5); // недопринятое — не в следующий ответ
				LL_USART_ClearFlag_ORE(UART5);
				return 1;
			}
		}
		rx_settings[i] = (uint8_t)LL_USART_ReceiveData9(UART5);
	}
	return 0;
}

int UART4_receive_timeout_10us() {
	for (uint8_t i = 0; i < rx_settings_length; i++) { // Таймаут 10us (275 тиков таймера TIM2 на 275MHz)
		TIM2->CNT = 0;
		const uint32_t timeout = 15 * 275; // 10us * 275 тиков/us

		while (!LL_USART_IsActiveFlag_RXNE(UART5)) {
			if (TIM2->CNT >= timeout) { // Время ожидания истекло, выходим без чтения
				LL_USART_ClearFlag_ORE(UART5);
				return 1;
			}
		}
		rx_settings[i] = (uint8_t)LL_USART_ReceiveData9(UART5);
	}
	return 0;
}

// DMA IQR Handler
// Шина стоит (TIM1 выключен), пока идёт этот обработчик: здесь только пакет в кольцо, MIDI считает и шлёт usb_task (PendSV)
void DMA1_RX(void) {


	LL_DMA_ClearFlag_TC2(DMA1);
	LL_TIM_DisableCounter(TIM1); // PWM - tim clk


	if (LL_USART_IsActiveFlag_NE(UART5)) {
		LL_USART_ClearFlag_NE(UART5);
		USART_noise_error_detected();
	}
	else {

		SCB_InvalidateDCache_by_Addr(rx_data, sizeof(rx_data)); // только своя строка: DMA записал RAM мимо кэша

		const uint32_t p = (uint32_t)rx_data[0] << 24 | rx_data[1] << 16 | rx_data[2] << 8 | rx_data[3];

		// const uint32_t tim_compare = TIM3->CNT;
		// if (tim_compare != rx_data[0] / count_comparators) { // TODO сделать проверку номера чипа!
		// 	debugg_fn(std::format(" chip number err {} != {}", rx_data[0], tim_compare));
		// }

		if (rx_data[0] > 168) { // DEBUG
			USART_noise_error_detected();
		}

		const uint32_t w = g4_ring_w;
		if (w - g4_ring_r < g4_ring_size) {
			g4_ring[w % g4_ring_size] = p;
			g4_ring_w = w + 1; // после записи пакета: usb_task видит индекс — пакет уже на месте
			SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
		}
		else {
			g4_ring_lost = g4_ring_lost + 1;
		}
	}

	LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_2);
	LL_TIM_EnableCounter(TIM1); // PWM - tim clk

}

// PendSV: единственный контекст TinyUSB, кроме tud_int_handler в OTG_HS. Будят OTG_HS (события стека) и DMA1_RX (пакет G4).
// Основной цикл TinyUSB не трогает: иначе DMA1_RX/PendSV врезаются в tud_task посреди захвата endpoint'а.
void usb_task() {
	tud_task(); // первым: завершённая передача освобождает IN, и нота ниже уйдёт с ближайшим опросом хоста, а не со следующим
	// OUT от хоста не нужен, но вычитывать обязательно: TinyUSB заряжает OUT, только когда FIFO приёма (64 Б) пуст,
	// иначе хост получает NAK, и запись в этот порт у хоста встаёт (DAW шлёт clock, sysex всем портам)
	uint8_t packet[4];
	while (tud_midi_packet_read(packet)) {
	}
	// Хост не слушает (не подключено, спит, порт не открыт — по USB видно лишь, что IN не забирают): FIFO передачи
	// очищаем, события выбрасываем, в midi_lost не считаем. Иначе старые ноты уйдут пачкой, когда хост снова начнёт читать
	const uint32_t now = HAL_GetTick();
	const uint32_t tx_done = tud_midi_n_tx_done(0);
	if (!tud_mounted() || tud_suspended()) {
		if (host_reads) {
			host_reads = false;
			tud_midi_n_tx_clear(0);
		}
	}
	else if (tx_done != host_tx_done_seen) { // хост забрал передачу
		host_tx_done_seen = tx_done;
		host_tx_edge_ms = now;
		host_reads = true;
	}
	else if (now - host_tx_edge_ms >= host_read_timeout_ms) {
		if (tud_midi_n_tx_probe(0)) { // IN свободен — проба
			host_tx_edge_ms = now;
		}
		else if (host_reads) { // передача висит — хост не читает
			host_reads = false;
			tud_midi_n_tx_clear(0);
		}
	}
	while (g4_ring_r != g4_ring_w) {
		const uint32_t r = g4_ring_r;
		if (host_reads) {
			g4_to_midi(g4_ring[r % g4_ring_size]);
		}
		g4_ring_r = r + 1;
	}
}

// пакет G4 → MIDI (из usb_task). p: номер компаратора << 24 | 24 бита времени
void g4_to_midi(const uint32_t p) {

	const uint32_t rxB = p >> 24;

	midi_hi_F = 0.0f;
	midi_lo_F = 0.0f;
	// timer_data_in = 0;


	uint32_t tOut = 0;
	tOut = p & 0xFFFFFF;
	const float t_F = (float)tOut / g4_ticks_per_us; // мкс, как кривые
	float integerPart_F;
	uint8_t note_ = rxB + noteAdder[rxB];

	// G4 OFF: бит 23 — подъём демпфера, младшие 23 — интервал R↓→G↓ (0 = не замерен).
	// Только вместе с SEND_LIFT 1 в G4: старый G4 шлёт обрезанное время, и бит 23 у него случаен.
	if (rxB >= 98 && send_damper_lift && (tOut & 0x800000)) {
		uint8_t v = 1; // не замерен — неглубокое/медленное нажатие
		if (tOut & 0x7FFFFF) {
			const float e = speed_vel((float)(tOut & 0x7FFFFF) / g4_ticks_per_us, lift_t_fast, lift_t_slow, curve_lift_selected);
			v = e < 1.0f ? 1 : (e > 127.0f ? 127 : (uint8_t)e);
		}
		uint8_t lift_buf[] = { 0xA0, note_, v }; // Poly Key Pressure: подъём демпфера, data2 — скорость подъёма
		midi_write(lift_buf, 3);
		return;
	}

	if (rxB >= 98) { // for OFF // 98 = чип 14, компаратор 0 — уже OFF
		midi_hi_F = speed_vel(t_F, off_t_fast, off_t_slow, curve_off_selected); // LEN_NONE (без замера) → 1
		midi_lo_F = modf(midi_hi_F, &integerPart_F) * maxMidi_F;
	}
	else { // for ON
		midi_hi_F = on_vel(curve_on_selected, t_F);
		midi_lo_F = modf(midi_hi_F, &integerPart_F) * maxMidi_F;
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

	// Событие целиком или никак: иначе уйдёт CC88 без ноты, а у верхних нот — note-on без note-off (нота зависнет).
	// В USB-MIDI каждое сообщение — пакет 4 Б: CC88 + нота = 8, с note-off — 12
	const uint32_t need = (rxB > 76 && rxB < 98) ? 12 : 8;
	if (tud_midi_n_tx_available(0) < need) {
		midi_lost = midi_lost + 1;
	}
	else {
		uint8_t note_buf[] = {
			0xB0,
			0x58,
			(uint8_t)midi_lo_F,
			(uint8_t)rxB < 98U ? 0x90U : 0x80U, // 0x90 note on
			note_,
			(uint8_t)midi_hi_F
		};

		midi_write(note_buf, 6);

		if (rxB > 76 && rxB < 98) { // верхние ноты без демпферов: release velocity не замеряется — 64 по стандарту MIDI
			uint8_t note_buff[] = { 0x80, note_, 64 };
			midi_write(note_buff, 3);
		}

		if (rxB < 98) {
			last_note = note_;
			last_note_vel = (uint8_t)midi_hi_F;
			last_note_t_us = (uint32_t)t_F;
			last_note_seq = last_note_seq + 1;
		}
	}

	if (rxB < 98) { // DEBUG
		if (t_F > max) {
			max = t_F;
			fl = 1;
		}
		if (t_F < min) {
			min = t_F;
			fl = 1;
		}
	}

	// speed_F = tOut; // DEBUG
	// timerLenght_F = midi_hi_F; // DEBUG
	// speed_F = midi_hi_F; // DEBUG
	// fl = rxB < 98 ? 3 : 0; // DEBUG // разрешить обновлять цифры на дисплее
}

void USART_noise_error_detected() { // из DMA1_RX: без std::format/кучи — печатает основной цикл
	for (uint32_t i = 0; i < dataLengthRX; ++i) {
		noise_bytes[i] = rx_data[i];
	}
	noise_count = noise_count + 1;
	LL_USART_RequestRxDataFlush(UART5);
	SCB_CleanInvalidateDCache();
}

void midi_write(const uint8_t* b, const uint32_t n) {
	if (tud_midi_stream_write(0, b, n) != n) {
		midi_lost = midi_lost + 1; // FIFO TinyUSB полон: событие (или его часть) потеряно
	}
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

	// кривая user в том же секторе: последнюю сохранённую — после стирания в начало журнала
	user_curve_rec curve_rec = {};
	const user_curve_rec* saved = user_curve_saved();
	const bool has_curve = saved != nullptr;
	if (has_curve) curve_rec = *saved;

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

	if (has_curve && !flash_program(ADDRESS_H7_USER_CURVE, (const uint32_t*)&curve_rec, sizeof(curve_rec) / 4)) {
		debugg_fn(" FLASH err 3 ");
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
	if (std::count(debugg.begin(), debugg.end(), '\n') >= 29) debugg.erase(0, debugg.find('\n') + 1); // окно 30 строк (экран «Журнал»): уходит самая старая
	if (!debugg.empty()) debugg += "\n";
	debugg += std::to_string(debug_counter);
	debugg += "        ";
	debugg += str;
	++debug_counter;
}

void debugg_printf(const char* fmt, ...) {
	char buf[96];
	va_list va;
	va_start(va, fmt);
	lv_vsnprintf(buf, sizeof(buf), fmt, va);
	va_end(va);
	debugg_fn(buf);
}

void debugg_clear() {
	debugg.clear();
}

void resetPin() {
	NVIC_SystemReset();
}

// LVGL UTILITES
// не используется: шёл в паре с неверной FT6336_ReadRegister_DMA, тач читается опросом
/*
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
*/

// velocity OFF и LIFT по скорости d/t, без массы: t_fast → 127, t_slow → 1. Скорости у демпфера лишь 16:1 (LIFT),
// и у ON-шного 127 · t_fast / t медленный конец не опускался бы ниже 8 — поэтому lin здесь от v_min, а не от нуля.
float speed_vel(const float t, const float t_fast, const float t_slow, const bool log) {
	if (log) return 1.0f + 126.0f * logf(t_slow / t) / logf(t_slow / t_fast); // ln(v / v_min) / ln(v_max / v_min)
	return 1.0f + 126.0f * (t_fast / t - t_fast / t_slow) / (1.0f - t_fast / t_slow); // (v − v_min) / (v_max − v_min)
}

// velocity ON кривой curve_num (номер в роллере) при пролёте t, мкс, без ограничения 1..127.
// Зовут g4_to_midi (PendSV) и график экрана кривых
float on_vel(const uint32_t curve_num, const float t) {
	if (curve_num == curve_lin_speed) return 127.0f * vel_t_fast / t; // v / v_max = t_fast / t
	if (curve_num == curve_log_speed) return 1.0f + 126.0f * logf(vel_t_slow / t) / logf(vel_t_slow / vel_t_fast); // ln(v / v_min) / ln(v_max / v_min)
	if (curve_num == curve_user_lin) return speed_vel(t, user_lin_t_fast, user_lin_t_slow, false);
	if (curve_num == curve_user_log) return speed_vel(t, user_log_t_fast, user_log_t_slow, true);
	if (curve_num == curve_user) { // как curve_ON, коэффициенты — с экрана кривой
		const float d = t + user_x0;
		return user_K / (d * d) + user_y0;
	}
	if (curve_num) {
		const speed_for_midi& curve = curve_ON[curve_num];
		const float speed = curve.distance / (t + curve.offset_x);
		return ((curve.key_mass * speed * speed) / 2.0f) + curve.offset_y;
	}
	// 0 — older curve: v1 старые значения, для использования вместе с программной-rust кривой на компе
	const float key_mass = 0.008f; // 8 гр
	const float distance_F = 0.0017f; // 1.7 мм (толщина шаблонов 1.9 и 0.2)
	const float div_on = 0.00000000089371f; // меньше - громче; на t в мкс (было 0.000000000092f на тики 102.9 нс)
	const float speed = distance_F / (t * div_on);
	return (key_mass * speed * speed) / deriv_F / maxMidi_F;
}

// K и offset_y — из концов: K / (t_fast + x0)² + y0 = 127, K / (t_slow + x0)² + y0 = 1
void user_curve_set() {
	const float x0 = user_curve.x0;
	const float a = 1.0f / ((user_curve.t_fast + x0) * (user_curve.t_fast + x0));
	const float b = 1.0f / ((user_curve.t_slow + x0) * (user_curve.t_slow + x0));
	const float K = 126.0f / (a - b);
	__disable_irq(); // g4_to_midi (PendSV) не должен застать K от новой кривой, а x0 от старой
	user_K = K;
	user_y0 = 1.0f - K * b;
	user_x0 = x0;
	__enable_irq();
}

// Журнал кривой user (ADDRESS_H7_USER_CURVE): сохранение дописывает запись за последней, не стирая сектор.
// Стирание (~1–2 с) останавливает чтение flash, а с ним прерывания и MIDI — оно нужно, только когда журнал полон.
// Индекс первой свободной записи; USER_CURVE_SLOTS — журнал полон
uint32_t user_curve_free_slot() {
	const user_curve_rec* rec = (const user_curve_rec*)ADDRESS_H7_USER_CURVE;
	uint32_t i = 0;
	while (i < USER_CURVE_SLOTS && rec[i].magic != 0xFFFFFFFF) {
		++i;
	}
	return i;
}

// последняя сохранённая запись; nullptr — записей нет, или она вне пределов ползунков
const user_curve_rec* user_curve_saved() {
	const uint32_t slot = user_curve_free_slot();
	if (slot == 0) return nullptr;
	const user_curve_rec* rec = (const user_curve_rec*)ADDRESS_H7_USER_CURVE + slot - 1;
	if (rec->magic != user_curve_magic) return nullptr;
	if (!(rec->t_fast >= user_t_fast_min && rec->t_fast <= user_t_fast_max)) return nullptr;
	if (!(rec->t_slow >= user_t_slow_min && rec->t_slow <= user_t_slow_max)) return nullptr;
	if (!(rec->x0 >= user_x0_min && rec->x0 <= user_x0_max)) return nullptr;
	return rec;
}

// words — кратно 8 (FLASHWORD, 32 Б). Сектор стёрт, flash отперта, кэши выключены — у вызывающего
bool flash_program(const uint32_t addr, const uint32_t* src, const uint32_t words) {
	for (uint32_t i = 0; i < words; i += 8) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, addr + i * sizeof(uint32_t), (uint32_t)&src[i]) != HAL_OK) {
			return false;
		}
	}
	return true;
}

bool user_curve_save() {
	uint32_t slot = user_curve_free_slot();
	bool ok = true;

	SCB_DisableICache();
	SCB_DisableDCache();
	HAL_FLASH_Unlock();

	if (slot == USER_CURVE_SLOTS) {
		// журнал полон: стереть сектор и вернуть калибровку как она лежит во flash, а не из RAM —
		// там могут быть несохранённые правки. buffer_green и buffer_red во flash идут подряд (size_BUFFER кратно 8)
		static uint32_t calib[2 * size_BUFFER];
		memcpy(calib, (const void*)ADDRESS_H7_CALIB, sizeof(calib));
		FLASH_Erase_Sector(FLASH_SECTOR_7, FLASH_BANK_1, FLASH_VOLTAGE_RANGE_2);
		ok = flash_program(ADDRESS_H7_CALIB, calib, 2 * size_BUFFER);
		slot = 0;
	}
	if (ok) {
		ok = flash_program(ADDRESS_H7_USER_CURVE + slot * sizeof(user_curve_rec), (const uint32_t*)&user_curve, sizeof(user_curve_rec) / 4);
	}

	HAL_FLASH_Lock();
	SCB_EnableICache();
	SCB_EnableDCache();
	return ok;
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

// Можно ли шить: все чипы в одном загрузчике, он не сотрёт сам себя, а образ в слоте H7
// собран под target. Иначе после сброса или прыжка G4 исполняет чужой код — только SWD.
bool g4_flash_allowed(const uint32_t& image_in_H7, const uint32_t& count_page, const uint32_t& target) {
	uint32_t running = 0; // начало работающего загрузчика G4
	uint32_t running_pages = 0;
	if (chip_state == chip_states::boot1) {
		running = ADDRESS_G4_BOOTLOADER;
		running_pages = COUNT_PAGE_G4_FOR_BOOTLOADER;
	}
	else if (chip_state == chip_states::boot2) {
		running = ADDRESS_G4_BOOTLOADER_v2;
		running_pages = COUNT_PAGE_G4_FOR_BOOTLOADER_v2;
	}
	else {
		debugg_fn(" flash: chips NOT BOOTLOADER ");
		return false;
	}
	for (const auto& chip : vChips) {
		if (chip.st != chip_state) {
			debugg_printf(" flash: chip %d in other state", chip.number_chip);
			return false;
		}
	}

	const uint32_t size = count_page * 0x800;
	if (target < running + running_pages * 0x800 && running < target + size) {
		debugg_printf(" flash: %" LV_PRIx32 " = running bootloader", target);
		return false;
	}

	// вектор образа: SP в RAM G4 (128K), Reset_Handler внутри target
	const uint32_t sp = ((volatile uint32_t*)image_in_H7)[0];
	const uint32_t reset = ((volatile uint32_t*)image_in_H7)[1];
	if (sp <= 0x20000000 || sp > 0x20020000 || reset < target || reset >= target + size) {
		debugg_printf(" flash: image not for %" LV_PRIx32 "\n SP %" LV_PRIx32 " reset %" LV_PRIx32, target, sp, reset);
		return false;
	}
	return true;
}

void data_from_H7_to_g4(const uint32_t& adress_g4_firmware_in_H7, const uint32_t& count_page, const uint32_t& addr_jump) {

	uint32_t start_adress_memory_read = adress_g4_firmware_in_H7; //  ++0x800 с каждым шагом
	uint32_t mem = addr_jump;
	std::string ships_ok = "ships flash ok .. ";
	int bug = 0;

	init_chips();
	if (!g4_flash_allowed(adress_g4_firmware_in_H7, count_page, addr_jump)) {
		return;
	}

	for (const auto& chip : vChips) {
		start_adress_memory_read = adress_g4_firmware_in_H7;
		mem = addr_jump;

		for (uint32_t ii = 0; ii < count_page; ++ii) {
			UART4_send_address(chip.number_chip);
			Set_tx_s((uint8_t)bootloader_command::data_from_H7_to_array_g4, 0x11, 0x12, 0x13, 0x14);
			UART4_send_settings_bootloader();

			// теперь внутри    From_H7_to_array_g4();
			// *  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  *
			int timeout = UART4_receive_settings_bootloader(); // >> 0x11, 0x12, 0x13, 0x14
			// -   - -   - - -   - -   - - -   - -   - - -   - -   - - -   - -   - - -   - -   - - -

			uint32_t primask = __get_PRIMASK();
			volatile uint32_t* pFlashAddr = (volatile uint32_t*)start_adress_memory_read;
			pause(1);
			for (uint32_t i = 0; i < 512 && !timeout; ++i) { // 2kB (4*512) размер пакета с прошивкой для отправки в g4
				uint32_to_bytes_pointer(pFlashAddr[i], tx_settings);
				tx_settings[4] = (uint8_t)i;
				pause(1);
				UART4_send_settings_bootloader();
				timeout = UART4_receive_settings_bootloader();
				if (bytes_to_uint32_pointer(rx_settings) != bytes_to_uint32_pointer(tx_settings)) {
					++bug;
				}
			}

			__set_PRIMASK(primask);
			if (timeout || UART4_receive_settings_bootloader()) { // response::ok
				debugg_printf(" flash: chip %d timeout at %" LV_PRIx32, chip.number_chip, mem);
				return;
			}
			pause(2);

			// _+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+_
			// _+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+_
			// _+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+__+_+_+_

			if (flash_g4(mem, chip.number_chip)) {
				debugg_printf(" flash: chip %d stop at %" LV_PRIx32, chip.number_chip, mem);
				return;
			}

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
		debugg_printf("%d -- copy bugs", bug);
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

int flash_g4(const uint32_t& addr, const int& chip_number) { // 1 — страница не записана: тайм-аут или отказ G4


	UART4_send_address(chip_number);
	Set_tx_s((uint8_t)bootloader_command::copy_array_to_flash_g4, 0x61, 0x62, 0x63, 0x64);
	UART4_send_settings_bootloader();

	// теперь внутри From_array_g4_to_H7();
	if (UART4_receive_settings_bootloader()) { // принимает ответ 0x15 0x61 0x62 0x63 0x64
		return 1;
	}

	// **  ****  ****  ****  ****  ****  ****  ****  ****  **
	pause(2);

	// 1 отправка адреса
	uint32_to_bytes_pointer(addr, tx_settings);
	tx_settings[4] = chip_number; // просто так ..
	UART4_send_settings_bootloader();

	// 2 принять адрес для проверки
	if (UART4_receive_settings_bootloader()) {
		return 1;
	}
	pause(1);
	uint32_t addr_back = bytes_to_uint32_pointer(rx_settings);
	if (addr_back != addr) {
		debugg_printf("  addr  fail  %" LV_PRIx32, addr_back);
		Set_tx_s((uint8_t)response::fail, 0x55, 0x56, 0x57, 0x58);
		UART4_send_settings_bootloader();
		UART4_receive_settings_bootloader(); // без записи G4 сразу шлёт итог (4), без 32 и 35
		return 1;
	}

	// 3 если ок - то разрешаем запись
	Set_tx_s((uint8_t)response::ok, 0x45, 0x46, 0x47, 0x48);
	UART4_send_settings_bootloader();

	//3.2 32, 3.5 35, 4 fail or ok
	if (UART4_receive_settings_bootloader() || UART4_receive_settings_bootloader() || UART4_receive_settings_bootloader()) {
		return 1;
	}

	if (rx_settings[1] != (uint8_t)response::ok) {
		debugg_printf("  FLASH G4 fail  ((  %d", chip_number);
		return 1;
	}
	return 0;
}

// Эхо чипу, пока в ответе rx_settings[i] != value: ждёт, когда G4 перезапустится или
// перейдёт в другую прошивку. 1 — не ответил за ~1.3 с (не висим на сломанном чипе)
int echo_until(const uint8_t& chip_N, const uint32_t& i, const uint8_t& value) {
	rx_settings[i] = (uint8_t)~value;
	for (uint32_t n = 0; n < 10000; ++n) {
		UART4_send_address(chip_N);
		Set_tx_s((uint8_t)bootloader_command::echo, 0x88, 0x88, 0x88, 0x88);
		UART4_send_settings_bootloader();
		UART4_receive_timeout_10us();
		if (rx_settings[i] == value) {
			return 0;
		}
		pause(100);
	}
	debugg_printf(" chip %d no echo", chip_N);
	return 1;
}

void jump_g4s_to_adress(const uint32_t& addr_jump, const chip_states& jump_to_) {

	init_chips();
	int bug = 0;

	for (const auto& chip : vChips) {

		UART4_send_address(chip.number_chip);
		Set_tx_s((uint8_t)bootloader_command::jump_g4_to_adress, 0x88, 0x88, 0x88, 0x88);
		UART4_send_settings_bootloader();
		if (UART4_receive_settings_bootloader() || UART4_receive_settings_bootloader()) { // принять (ChipN_32, 0x88, 0x88, 0x88, 0x88)
			debugg_printf(" jump: chip %d timeout", chip.number_chip);
			++bug;
			continue; // сломанный чип не держит остальные (и старт H7)
		}

		if (bytes_to_uint32_pointer(&rx_settings[1]) != bytes_to_uint32_pointer(&tx_settings[1])) {
			++bug;
		}
		pause(4);

		// отправка адреса
		uint32_to_bytes_pointer(addr_jump, tx_settings);
		tx_settings[4] = chip.number_chip;
		UART4_send_settings_bootloader();
		if (UART4_receive_settings_bootloader()) { // принять (полученный g4 адрес)
			debugg_printf(" jump: chip %d timeout", chip.number_chip);
			++bug;
			continue;
		}
		uint32_t addr_back = bytes_to_uint32_pointer(rx_settings);
		pause(1);

		// если ок, то прыгаем
		if (addr_back == addr_jump) {
			Set_tx_s((uint8_t)response::ok, 0x03, 0x02, 0x01, 0x00);
			UART4_send_settings_bootloader();
		}
		else {
			debugg_printf("  jumping addr  bug  %" LV_PRIx32 " != %" LV_PRIu32, addr_back, addr_jump);
		}
		int timeout = UART4_receive_settings_bootloader();

		if (jump_to_ == chip_states::boot2) {
			timeout = echo_until(chip.number_chip, 1, 0);
		}
		else if (jump_to_ == chip_states::boot1) {
			timeout = echo_until(chip.number_chip, 1, 0);
		}
		else if (jump_to_ == chip_states::piano && !timeout) {
			timeout = UART4_receive_settings_bootloader();
		}

		if (timeout || rx_settings[0] != chip.number_chip) {
			debugg_printf(" start %d fail \n", chip.number_chip);
		}

		pause(2);
	}

	if (bug) {
		debugg_printf(" jump to address FAIL %d bugs", bug);
		return;
	}
}

void reset_bootloaders() {

	if (chip_state == chip_states::boot1) {
		for (const auto& chip : vChips) {
			UART4_send_address(chip.number_chip);
			Set_tx_s((uint8_t)bootloader_command::reset, 0x04, 0x03, 0x02, 0x01); // 200ms delay
			UART4_send_settings_bootloader();
			if (UART4_receive_settings_bootloader()) {
				debugg_printf(" reset: chip %d timeout", chip.number_chip);
				continue;
			}
			echo_until(chip.number_chip, 4, 1); // ждём v1 после сброса
		}
	}
	else if (chip_state == chip_states::boot2) {
		for (const auto& chip : vChips) {
			UART4_send_address(chip.number_chip);
			Set_tx_s((uint8_t)bootloader_command::reset, 0x04, 0x03, 0x02, 0x01); // 200ms delay
			UART4_send_settings_bootloader();
			if (UART4_receive_settings_bootloader()) {
				debugg_printf(" reset: chip %d timeout", chip.number_chip);
				continue;
			}
			echo_until(chip.number_chip, 4, 1); // ждём v1 после сброса
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
			echo_until(chip.number_chip, 4, 1); // ждём v1 после сброса
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
