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
#include "gui.hpp"

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
		read_comp_setting,
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
		none = 0,
		boot1 = 1,
		piano,
		boot2
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

	struct speed_for_midi { // mass, distance, offset_X, offset_Y
		const float key_mass = 0.0f; // масштаб энергии → velocity (не граммы)
		const float distance = 0.0f; // мкм: 1710 == 1.71 mm; мкм / мкс = м/с
		const float offset_x = 0.0f; // offset x, мкс
		const float offset_y = 0.0f; // offcet y
	};

	// G4 считает интервал тиками TIM2 с PSC 0 — 1/136 МГц ≈ 7.35 нс (TICK_US в G4). t_F и все времена ниже —
	// в микросекундах. Делим во float: дробная часть — та точность, что нужна hi-res velocity.
	const float g4_ticks_per_us = 136.0f;

	// Кривые ON «по скорости» (роллер 9, 10): velocity — функция скорости d/t, без массы и сдвигов.
	// Границы — в мкс, как t_F: vel_t_fast → 127 в обеих; vel_t_slow → 1 в log
	// (в lin медленный конец не задаётся: 127 · t_fast / t, при vel_t_slow ≈ 1.5).
	const uint8_t curve_lin_speed = 9; // velocity ∝ скорость: для piano_sim (v0 ∝ velocity, нужен 14 бит)
	const uint8_t curve_log_speed = 10; // velocity ∝ ln скорости: для синтезаторов без hi-res velocity
	const float vel_t_fast = 400.0f;   // 339.7 // мкс — 127 у реального ff; рекорд 298.0 мкс — разброс датчиков
	const float vel_t_slow = 28411.8f; // мкс — по max из замеров (28394.8, окно G4 было 30 мс)

	// Кривая ON «user» (роллер 11) — формула curve_ON: K / (t + x0)² + offset_y, K = key_mass · distance² / 2.
	// Задаётся на экране кривой тем, что слышно: t_fast → 127, t_slow → 1, x0 — изгиб (больше — громче середина).
	// K и offset_y считает user_curve_set()
	const uint8_t curve_user = 11;
	struct user_curve_rec { // запись во flash — ровно один FLASHWORD
		uint32_t magic;
		float t_fast; // мкс
		float t_slow; // мкс
		float x0;     // мкс
		uint32_t reserved[4];
	};
	static_assert(sizeof(user_curve_rec) == 32, "запись кривой user пишется одним FLASHWORD");
	const uint32_t user_curve_magic = 0x55435256;
	// Пределы ползунков, мкс: t_fast + x0 ≥ 50 — знаменатель не обнуляется; t_fast < t_slow при любых значениях
	const int32_t user_t_fast_min = 200;
	const int32_t user_t_fast_max = 2000;
	const int32_t user_t_slow_min = 5000;
	const int32_t user_t_slow_max = 30000;
	const int32_t user_x0_min = -150;
	const int32_t user_x0_max = 10000;
	// «user lin» и «user log» (роллер 12, 13) — speed_vel, как OFF и LIFT: t_fast → 127, t_slow → 1, между ними
	// ∝ скорости или её логарифму. В отличие от lin speed (127 · t_fast / t) медленный край задан и не уезжает за t_fast.
	// Правятся на экране кривой, но не сохраняются: при старте и по reset — vel_t_fast / vel_t_slow. Опора, с которой сравнивать user
	const uint8_t curve_user_lin = 12;
	const uint8_t curve_user_log = 13;

	// OFF (G↑→R↑, демпфер ложится) и LIFT (R↓→G↓, клавиша поднимает демпфер) — свои карты скорости,
	// роллер ON на них не влияет. Роллеры roller_off / roller_lift: 0 — lin (∝ скорости, t_slow → 1), 1 — log (как log speed).
	// Границы — по замеру min/max, мкс (off 2067.8..61680.9, lift 957.8..15138.9).
	const float off_t_fast = 2058.8f;   // мкс
	const float off_t_slow = 61680.9f;  // мкс — насыщение G4 (LEN_MAX / 136): медленнее не замерить
	const float lift_t_fast = 957.4f;   // мкс
	const float lift_t_slow = 15132.4f; // мкс — OFF_LIFT_WAIT в G4: дольше подъём идёт без замера


	// Переменные — объявления: определения в piano_h7.cpp, заголовок включает ещё и gui.cpp

	// ***** 390(263)-14000(22723)us пролёт молоточка
	// Приёмник DMA1 занимает целую строку D-cache (32 байта): CPU сюда не пишет, строка
	// никогда не бывает «грязной», и очистка кэша не затрёт свежий пакет старой копией.
	extern uint8_t rx_data[32];
	const uint32_t dataLengthRX = 4;
	extern uint8_t tx_settings[5];
	const uint32_t tx_settings_length = sizeof(tx_settings);
	extern uint8_t rx_settings[5];
	const uint32_t rx_settings_length = sizeof(rx_settings);
	extern uint8_t compN_;
	extern uint8_t dot_;
	extern uint8_t a_;
	extern uint8_t b_;

	// new variants
	// --()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()

	static const uint32_t ADDRESS_H7_BOOTLOADER = 0x08000000;
	static const uint32_t ADDRESS_H7_MAIN_FIRMWARE = 0x08020000; // размер +- 0x0008E064 до ~0x080AE070 до 5го блока включительно
	static const uint32_t ADDRESS_H7_MAIN_FIRMWARE_FOR_G4 = 0x080C0000; // хватает ли места для размещения прошивки? (6й блок) ~0x2f40 размер
	static const uint32_t ADDRESS_H7_CALIB = 0x080E0000; // -здесь лежит калибровка
	static const uint32_t ADDRESS_H7_USER_CURVE = 0x080F0000; // вторая половина сектора 7 — журнал записей кривой user
	static const uint32_t USER_CURVE_SLOTS = 0x10000 / sizeof(user_curve_rec);

	static const uint32_t ADDRESS_G4_BOOTLOADER = 0x08000000; // 8k (4 банки)
	static const uint32_t COUNT_PAGE_G4_FOR_BOOTLOADER = 4;
	static const uint32_t ADDRESS_G4_BOOTLOADER_v2 = 0x08002000; // 8k (4 банки)
	static const uint32_t COUNT_PAGE_G4_FOR_BOOTLOADER_v2 = 4;
	static const uint32_t ADDRESS_G4_CHIP_NUMBER = 0x08004800; // здесь храним номер чипа (в памяти g4) 9я банка // TODO изменено!
	static const uint32_t ADDRESS_G4_MAIN_FIRMWARE = 0x08008000; // этот адрес зашит в памяти g4 (16я банка) - сейчас размер на 8 банок.
	static const uint32_t ADDRESS_G4_MAIN_FIRMWARE_ALT = 0x08008000; // здесь меняем куда шить и куда прыгать
	static const uint32_t COUNT_PAGE_FOR_FIRMWARE_G4 = 8;// количество страниц (8) в g4, которые занимает прошивка g4 (16-23)


	const uint32_t allChipCount = 27; // 1-13-on, 14-23(26)-off // до этого значения считает таймер 
	const uint32_t on_off_division = 14; // точка разделения on-off

	const uint32_t size_BUFFER = 200; // кратно 8
	static_assert(size_BUFFER % 8 == 0, "save_to_memory пишет FLASHWORD по 8 элементов, read_on_memory так же считает смещение buffer_red");
	const uint32_t buffer_division = size_BUFFER / 2;
	const uint32_t count_comparators = 7;

	extern int32_t buffer_green[size_BUFFER];
	extern int32_t buffer_red[size_BUFFER];
	extern int32_t buffer_blue_calib[size_BUFFER];
	extern int32_t buffer_blue_calib_old[size_BUFFER];
	extern int32_t buffer_dac[size_BUFFER];

	struct default_comp {
		int32_t green_on_default = 2601;
		int32_t red_on_default = 1001;
		int32_t green_off_default = 2102;
		int32_t red_off_default = 2402;
	};
	extern struct default_comp def_comp;

	struct default_graph_min_max {
		int32_t on_min = 4095;
		int32_t on_max = 0;
		int32_t off_min = 0;
		int32_t off_max = 4095;
	};
	extern default_graph_min_max graph_min_max;

	extern std::vector<Chip> vChips;
	extern std::map<uint8_t, comparator> mCursor_to_comparator_on;
	extern std::map<uint8_t, comparator> mCursor_to_comparator_off;

	extern chip_states chip_state;
	extern chip_states chip_state_prev;
	extern current_display cur_disp;
	extern uint32_t cursor;
	extern bool calib_follow; // курсор калибровки сам переходит на клавишу, которая движется (update_cursor)
	extern uint32_t calib_dirty; // порогов задано после последнего сохранения во flash
	extern bool calib_data_new; // calib_poll принёс свежие значения — экрану калибровки перерисоваться

	extern int8_t noteAdder[size_BUFFER];
	// float mass_F[size_BUFFER] = {}; // TODO FLASH сделать сохранениее в память

	extern int counter_on;
	extern int counter_off;

	extern bool chip_silent[allChipCount]; // G4 не ответил на последнюю команду (печать «NO ANSWER» один раз)
	extern bool chip_bad_adress[allChipCount]; // последний ответ G4 — с чужим адресом (печать «BAD ADRESS» один раз)
	extern bool chip_bad_data[allChipCount]; // последний set_comp_value вернул не то значение (печать «BAD SET DATA» один раз)

	extern bool send_damper_lift; // подъём демпфера (бит 23 от G4) → Poly Key Pressure 0xA0; включать только вместе с SEND_LIFT 1 в G4

	extern volatile uint32_t noise_count; // ошибки шины G4 в DMA1_RX; печать — в основном цикле
	extern uint32_t noise_shown;
	extern volatile uint8_t noise_bytes[4];
	extern volatile uint32_t midi_lost; // событий, не влезших в FIFO TinyUSB (пишет usb_task)
	extern uint32_t midi_lost_shown;
	// Пакеты G4 из DMA1_RX в usb_task (PendSV): rx_data[0] << 24 | 24 бита времени. Пишет только DMA1_RX, читает только usb_task.
	// 64 — как FIFO TinyUSB (512 Б = 64 события по 8 Б); пакет по шине ≥ ~6 мкс — запас на ~0.4 мс простоя PendSV.
	// Все очереди G4 (26 чипов × 16) — 416 событий, но приходят они не быстрее шины, а PendSV их разбирает сразу.
	const uint32_t g4_ring_size = 64;
	static_assert((g4_ring_size & (g4_ring_size - 1)) == 0, "индексы идут по модулю 2^32 — размер должен быть степенью 2");
	extern volatile uint32_t g4_ring[g4_ring_size];
	extern volatile uint32_t g4_ring_w;
	extern volatile uint32_t g4_ring_r;
	extern volatile uint32_t g4_ring_lost; // кольцо полно — usb_task не успевает (пишет DMA1_RX)
	extern uint32_t g4_ring_lost_shown;
	// Читает ли хост IN (только usb_task). Открытый порт опрашивается каждые 30–90 мкс: передача, висящая 50 мс, —
	// хост не читает; IN, свободный 50 мс, — проба пустым пакетом. Проба висит ≤ 1 опроса, ноту задержит редко
	const uint32_t host_read_timeout_ms = 50;
	extern bool host_reads;
	extern uint32_t host_tx_done_seen;
	extern uint32_t host_tx_edge_ms; // HAL_GetTick() последней забранной передачи или поставленной пробы
	extern volatile uint32_t lcd_dma_errors; // ошибки DMA дисплея (пишет DMA2_Stream1); печать — в основном цикле
	extern uint32_t lcd_dma_errors_shown;
	extern uint32_t touch_errors_shown; // ft6336_errors на момент последней печати
	extern uint32_t touch_errors_tick;  // HAL_GetTick() последней печати
	// Последняя нота ON для строки на главном экране: пишет g4_to_midi (PendSV), читает основной цикл.
	// last_note_seq — после остальных: основной цикл видит новый номер — нота уже на месте
	extern volatile uint8_t last_note;
	extern volatile uint8_t last_note_vel;
	extern volatile uint32_t last_note_t_us;
	extern volatile uint32_t last_note_seq;

	// --()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()--()

	extern int fl; // for test fl
	extern float speed_F; // for test
	extern float midi_hi_F; // for test
	extern float midi_lo_F; // for test
	extern uint32_t out_debug_; // for test
	extern float timer_data_in;
	extern uint32_t debug_counter;
	const uint32_t key_to_change_memory = { 0xBAFC }; // allChipCount * 0x40 - смещение; 0xBAFC - просто код, который если изменить, то данные перезапишутся в памяти

	extern float min; // мкс; прежние, в тиках 102.9 нс: 3261 (3834) (~3600)
	extern float max; // мкс; прежние, в тиках 102.9 нс: 170106 (170000) 181000


	// const float key_mass = 0.008f; // 8 гр -->> переехал в массив
	// const float distance_F = 0.0017f; // 1.7 мм (толщина шаблонов 1.9 и 0.2)
	// const float div_on = 0.000000000092f; // меньше - громче
	// const float div_off = 0.00000000004f; // меньше - громче 
	const float deriv_F = 2.0f; // делить на 2 в формуле
	const float maxMidi_F = 127.99f;
	extern std::vector<speed_for_midi> curve_ON;
	// Кривая ON из роллера. DMA1_RX читает её отсюда, а не lv_roller_get_selected: LVGL нельзя звать
	// из прерывания, а до ui_init objects.curve_roller == NULL. 9 (lin speed) — как lv_roller_set_selected в screens.c
	extern volatile uint8_t curve_on_selected;
	extern volatile uint8_t curve_off_selected;  // OFF: 0 — lin, 1 — log
	extern volatile uint8_t curve_lift_selected; // LIFT: 0 — lin, 1 — log
	extern user_curve_rec user_curve; // ползунки; по умолчанию ≈ normal upd
	extern volatile float user_K;  // из user_curve; читает g4_to_midi (PendSV)
	extern volatile float user_y0;
	extern volatile float user_x0;
	extern volatile float user_lin_t_fast; // user lin, мкс
	extern volatile float user_lin_t_slow;
	extern volatile float user_log_t_fast; // user log, мкс
	extern volatile float user_log_t_slow;
	extern float m_F; // for test
	extern float sd_F; // for test
	extern float sx_F; // for test
	extern float sy_F; // for test

	void to_sleep();
	void init();
	void init_LL();
	void init_chips();
	void init_buffers();
	void init_midi_speeds();
	void reconfig_charts();
	void h7();
	void sync();
	int sync_sender(const uint8_t& i);
	void all_H7_to_g4();
	void all_g4_to_H7();
	void read_comp_setting(const typeAction& t);
	void update_cursor(const Chip& comp);
	void calib_enter(const typeAction t);
	void calib_leave();
	void calib_poll();
	void calib_set_threshold(const dot d);
	void calib_cursor_move(const int32_t delta);
	void calib_save();
	void calib_restore();
	void bus_stop();
	void bus_resume();
	void chips_rescan();
	bool sender(const command& com, const uint8_t& adress, const uint8_t& compN, const dot& dot, const uint32_t& value);
	void UART4_send_address(const uint8_t& slave_address);
	void UART4_send_settings(const command& com, const uint8_t& compN, const uint8_t& dot, const uint32_t& value);
	int UART4_receive_settings();
	void UART4_send_settings_bootloader();
	int UART4_receive_settings_bootloader();
	int UART4_receive_timeout_10us();
	void DMA1_RX();
	void usb_task();
	void g4_to_midi(const uint32_t p);
	void USART_noise_error_detected();
	void midi_write(const uint8_t* b, const uint32_t n);
	void DMA_UART_ERRORS_HANDLER();
	int32_t convert_8_16(const uint8_t& a, const uint8_t& b);
	conv_16_8 convert_16_8(const uint32_t& a);
	void save_to_memory();
	void read_on_memory();
	void pause(const uint32_t& p);
	void debugg_fn(const std::string& str);  // DEBUG
	void debugg_printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));  // DEBUG, через lv_snprintf
	void debugg_clear();
	void resetPin();
	// void DMA2_Stream3_i2c(); // не используется: DMA для тача отключён
	float speed_vel(const float t, const float t_fast, const float t_slow, const bool log);
	float on_vel(const uint32_t curve_num, const float t);
	void user_curve_set();
	uint32_t user_curve_free_slot();
	const user_curve_rec* user_curve_saved();
	bool flash_program(const uint32_t addr, const uint32_t* src, const uint32_t words);
	bool user_curve_save();
	void Read_uint32(uint32_t Address, volatile uint32_t* pData, uint32_t Size);
	void Set_tx_s(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e);
	bool g4_flash_allowed(const uint32_t& image_in_H7, const uint32_t& count_page, const uint32_t& target);
	int echo_until(const uint8_t& chip_N, const uint32_t& i, const uint8_t& value);
	void data_from_H7_to_g4(const uint32_t& adress_g4_firmware_in_H7, const uint32_t& count_page, const uint32_t& addr_jump);
	int flash_g4(const uint32_t& addr, const int& chip_number);
	void jump_g4s_to_adress(const uint32_t& addr_jump, const chip_states& jump_to_);
	void reset_bootloaders();
	void reset_main_to_bootloader();
	const comparator& searcher_addr_in_cursor(const uint32_t& chip);

	extern std::string debugg;

	// int32_t on_green_max = 4095;
	// int32_t on_green_min = 0;
	// int32_t on_red_max = 4095;
	// int32_t on_red_min = 0;
	// int32_t off_green_max = 4095;
	// int32_t off_green_min = 0;
	// int32_t off_red_max = 4095;
	// int32_t off_red_min = 0;

}
#endif // extern "C"
