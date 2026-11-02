/*
 * сцены.cpp — экраны для README: gui.cpp и ui/ как есть, вместо железа и piano_h7.cpp — заглушки ниже.
 * Собирает и запускает рендер.py. Запуск: сцены <сцена> <файл.ppm>
 *
 * Сцена задаёт состояние прошивки (платы, нота, пороги, журнал) и делает то же, что нажатия на экране:
 * action_* и события кнопок. Кадр LVGL шлёт через my_flush_cb в Send_DMA_Data8 — здесь он копируется в fb.
 * Данные — как в сценах eez_proj/макеты.py.
 */

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include "piano_h7.hpp"
#include "screens.h"

// --------------------------------------------------------------- железо

stub_periph stub_dma2, stub_spi3;
static uint32_t tick = 0;
uint32_t HAL_GetTick() { return tick; }
void request_power_off() {}

static uint8_t fb[320 * 480 * 3]; // RGB888 LVGL: в памяти B, G, R
static uint16_t win_x1, win_y1, win_x2, win_y2;
void LCD_Init() {}
void FT6336_Init() {}
TouchPoints_HandleTypeDef FT6336_GetTouchPoint() { return {}; }
void LCD_SetWindows(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
	win_x1 = x1;
	win_y1 = y1;
	win_x2 = x2;
	win_y2 = y2;
}
void Send_DMA_Data8(uint8_t* buff, uint16_t) {
	const uint32_t w = (win_x2 - win_x1 + 1) * 3;
	for (uint32_t y = win_y1; y <= win_y2; ++y) {
		memcpy(&fb[(y * 320 + win_x1) * 3], buff + (y - win_y1) * w, w);
	}
	DMA2_Stream1_TransferComplete(); // DMA «завершён» сразу
}
uint32_t ft6336_errors = 0;
int _bflag;

// --------------------------------------------------------------- состояние прошивки (piano_h7.cpp)

int32_t buffer_green[size_BUFFER];
int32_t buffer_red[size_BUFFER];
int32_t buffer_blue_calib[size_BUFFER];
int32_t buffer_dac[size_BUFFER];
default_graph_min_max graph_min_max;
std::vector<Chip> vChips;
std::map<uint8_t, comparator> mCursor_to_comparator_on;
std::map<uint8_t, comparator> mCursor_to_comparator_off;
chip_states chip_state = chip_states::none;
current_display cur_disp = current_display::none;
uint32_t cursor = 0;
bool calib_follow = true;
uint32_t calib_dirty = 0;
bool calib_data_new = false;
int8_t noteAdder[size_BUFFER];
volatile uint32_t noise_count = 0;
volatile uint32_t midi_lost = 0;
volatile uint32_t g4_ring_lost = 0;
bool host_reads = true;
volatile uint32_t lcd_dma_errors = 0;
volatile uint8_t last_note = 0;
volatile uint8_t last_note_vel = 0;
volatile uint32_t last_note_t_us = 0;
volatile uint32_t last_note_seq = 0;
uint32_t debug_counter = 0;
std::string debugg;
volatile uint8_t curve_on_selected = 9;
volatile uint8_t curve_off_selected = 0;
volatile uint8_t curve_lift_selected = 0;
user_curve_params user_curve = user_curve_default;
bool show_last_note = false;
volatile float user_K = 0.0f;
volatile float user_y0 = 0.0f;
volatile float user_x0 = 0.0f;
volatile float user_lin_t_fast = vel_t_fast;
volatile float user_lin_t_slow = vel_t_slow;
volatile float user_log_t_fast = vel_t_fast;
volatile float user_log_t_slow = vel_t_slow;

static comparator calib_comp;
const comparator& searcher_addr_in_cursor(const uint32_t&) { return calib_comp; }
void calib_enter(const typeAction t) { cur_disp = t == typeAction::on ? current_display::on : current_display::off; }

void calib_leave() {}
void calib_set_threshold(const dot) {}
void calib_cursor_move(const int32_t) {}
void calib_save() {}
void calib_restore() {}
void bus_stop() {}
void bus_resume() {}
void chips_rescan() {}
void init_chips() {}
void to_sleep() {}
void debugg_clear() {}
void reset_bootloaders() {}
void reset_main_to_bootloader() {}
void data_from_H7_to_g4(const uint32_t&, const uint32_t&, const uint32_t&) {}
void jump_g4s_to_adress(const uint32_t&, const chip_states&) {}
bool user_curve_save() { return true; }
void settings_save_selection() {}

// кривые — копия из piano_h7.cpp; curve_ON (роллер 1–8) не заполнен, сцены их не берут
float speed_vel(const float t, const float t_fast, const float t_slow, const bool log) {
	if (log) return 1.0f + 126.0f * logf(t_slow / t) / logf(t_slow / t_fast);
	return 1.0f + 126.0f * (t_fast / t - t_fast / t_slow) / (1.0f - t_fast / t_slow);
}

float on_vel(const uint32_t curve_num, const float t) {
	if (curve_num == curve_lin_speed) return 127.0f * vel_t_fast / t;
	if (curve_num == curve_log_speed) return 1.0f + 126.0f * logf(vel_t_slow / t) / logf(vel_t_slow / vel_t_fast);
	if (curve_num == curve_user_lin) return speed_vel(t, user_lin_t_fast, user_lin_t_slow, false);
	if (curve_num == curve_user_log) return speed_vel(t, user_log_t_fast, user_log_t_slow, true);
	if (curve_num == curve_user) {
		const float d = t + user_x0;
		return user_K / (d * d) + user_y0;
	}
	return 0.0f;
}

void user_curve_set() {
	const float x0 = user_curve.x0;
	const float a = 1.0f / ((user_curve.t_fast + x0) * (user_curve.t_fast + x0));
	const float b = 1.0f / ((user_curve.t_slow + x0) * (user_curve.t_slow + x0));
	const float K = 126.0f / (a - b);
	user_K = K;
	user_y0 = 1.0f - K * b;
	user_x0 = x0;
}

// --------------------------------------------------------------- сцены

static const char* const LOG =
	"0        *+*+*+*   piano   *+*+*+*\n1        calib H7 >> G4\n2        sync OK\n"
	"3         1 2 3 4 5 6 7 8 9 10 11 12 13 14 ... = 26\n         pianos\n";

static void chips(const chip_states st, const uint32_t boot1_chip = 0) {
	for (uint32_t n = 1; n < allChipCount; ++n) {
		Chip c;
		c.number_chip = n;
		c.typ = n < on_off_division ? typeAction::on : typeAction::off;
		c.st = n == boot1_chip ? chip_states::boot1 : st;
		vChips.push_back(c);
	}
}

static void log_text(const char* tail) {
	debugg = std::string(LOG) + tail;
	++debug_counter;
}

static void tick_gui() {
	tick += 20;
	gui_tick();
}

static void release(lv_obj_t* obj) {
	lv_obj_send_event(obj, LV_EVENT_RELEASED, nullptr);
}

// калибровка: клавиша 23 (G2, чип 4, компаратор 1); пороги по чипам — пилой, как идут компараторы
static void calib_data() {
	uint64_t s = 11;
	auto rnd = [&s]() { return (int32_t)((s = s * 16807 % 2147483647) % 160) - 80; };
	for (uint32_t i = 0; i < size_BUFFER; ++i) {
		const int32_t saw = (int32_t)(i % count_comparators) * 60;
		buffer_green[i] = 2050 + saw + rnd();
		buffer_red[i] = 1550 + saw + rnd();
		buffer_blue_calib[i] = 1800 + saw + rnd();
		noteAdder[i] = i < 55 ? 14 : i < 96 ? 13 : i < 146 ? -77 : -78;
	}
	calib_comp = { 0, 7 + 3 * 7 + 1, 4, 1, true };
	const uint32_t a = calib_comp.address;
	buffer_green[a] = 2101;
	buffer_red[a] = 1580;
	buffer_blue_calib[a] = 1834;
	buffer_blue_calib[a - 1] = 1790;
	buffer_blue_calib[a + 1] = 1851;
}

static bool scene(const char* name) {
	const std::string s = name;
	if (s == "1_main") {
		chips(chip_states::piano);
		gui_init();
		tick_gui();
	}
	else if (s == "1b_main_note") {
		chips(chip_states::piano);
		show_last_note = true;
		calib_dirty = 3;
		noise_count = 2;
		last_note = 69; // A4
		last_note_vel = 87;
		last_note_t_us = 3200;
		last_note_seq = 1;
		gui_init();
		tick_gui();
	}
	else if (s == "2_curve" || s == "2b_curve_off") {
		curve_on_selected = curve_user;
		user_curve_set();
		last_note_t_us = 1500; // перекрестье
		last_note_seq = 1;
		gui_init();
		action_to_curve(nullptr);
		if (s == "2b_curve_off") {
			release(objects.curve_tab_off);
		}
	}
	else if (s == "3_calib") {
		chips(chip_states::piano);
		calib_data();
		calib_dirty = 3;
		gui_init();
		action_to_calib(nullptr);
	}
	else if (s == "4_service") {
		chips(chip_states::piano, 9);
		noise_count = 2;
		gui_init();
		action_to_service(nullptr);
	}
	else if (s == "5_flash") {
		chips(chip_states::boot1);
		chip_state = chip_states::boot1;
		log_text("4        reset to bootloader\n5         1 2 3 4 5 6 7 8 9 10 11 12 13 14 ... = 26\n"
			"         bootloaders\n6        *+*+*+*   bootloader   *+*+*+*");
		gui_init();
		action_to_flash(nullptr);
	}
	else if (s == "6_log_confirm") {
		chips(chip_states::piano);
		noise_count = 2;
		log_text("4        calib G4 >> H7\n5        calib saved\n"
			"6        USART noise 1 : 0-0-255-0\n7        USART noise 2 : 0-0-255-0");
		gui_init();
		action_to_log(nullptr);
		action_jump_to_dfu(nullptr);
	}
	else {
		return false;
	}
	return true;
}

int main(int argc, char** argv) {
	if (argc != 3 || !scene(argv[1])) {
		fprintf(stderr, "сцены <сцена> <файл.ppm>\n");
		return 1;
	}
	for (int n = 0; n < 100; ++n) { // секунда: переходы стилей по состоянию успевают закончиться
		tick += 10;
		lv_tick_inc(10); // в прошивке — из SysTick
		lv_timer_handler();
	}
	lv_refr_now(nullptr);
	FILE* f = fopen(argv[2], "wb");
	fprintf(f, "P6\n320 480\n255\n");
	for (uint32_t p = 0; p < 320 * 480; ++p) {
		const uint8_t rgb[3] = { fb[p * 3 + 2], fb[p * 3 + 1], fb[p * 3] };
		fwrite(rgb, 1, 3, f);
	}
	fclose(f);
	return 0;
}
