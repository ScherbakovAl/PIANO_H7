/*
 * gui.cpp — интерфейс: порт LVGL, действия кнопок экранов EEZ (ui/) и всё, что в EEZ не задать
 *
 * Раскладка экранов — в eez_proj/display_piano.eez-project; ui/ генерирует EEZ Studio (Build).
 * Здесь — только то, что меняется во время работы: тексты, видимость, серии графиков, сетка плат.
 */

#include <cmath>
#include <cstdarg>
#include <cstring>
#include "vector"
#include "map"
#include "string"
#include "piano_h7.hpp"
#include "screens.h"
#include "fonts.h"

// --------------------------------------------------------------- порт LVGL

// настройки gpio для DISPLAY взяты отсюда: https://github.com/zeruns/STM32F407_LVGL_Template_MSP3526/blob/master/Core/Src/gpio.c
const uint32_t BYTES_PER_PIXEL = (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB888)); // 565 or 888? (было 565)
const uint32_t BUFF_SIZE = (320 * 60 * BYTES_PER_PIXEL); // 60 строк по 320 px = 57600 байт
static_assert(BUFF_SIZE <= 0xFFFF, "кусок уходит одним DMA: NDTR и Send_DMA_Data8 — 16 бит");
static_assert(LV_DRAW_BUF_STRIDE_ALIGN == 1, "my_flush_cb шлёт width * height * BYTES_PER_PIXEL подряд — строки не должны дополняться");
alignas(32) static uint8_t buf_1[BUFF_SIZE]; // выравнивание по строке D-cache — для SCB_CleanDCache_by_Addr
alignas(32) static uint8_t buf_2[BUFF_SIZE];

lv_display_t* disp;
static lv_indev_t* indev;
// Кусок кадра идёт по DMA (ставит my_flush_cb, снимает DMA2_Stream1). LVGL ждёт его только перед
// следующим flush, поэтому после lv_timer_handler он ещё может лететь: перед прямыми LCD_* — ждать
static volatile bool lcd_dma_busy = false;

static void init_LCD_and_touch() {

	// LCD init
	LL_SPI_EnableDMAReq_TX(SPI3); // TXDMAEN в CFG1 — до SPE (RM0468)
	LL_SPI_Enable(SPI3);
	LL_SPI_StartMasterTransfer(SPI3);
	LCD_Init();

	// TOUCH init
	// LL_I2C_Enable(I2C5);
	FT6336_Init();

}

static void my_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* color_p) {
	LCD_SetWindows(area->x1, area->y1, area->x2, area->y2);
	const int32_t height = area->y2 - area->y1 + 1;
	const int32_t width = area->x2 - area->x1 + 1;
	const int32_t wh_ = width * height * BYTES_PER_PIXEL; // кусок сплошной: строки без выравнивания (static_assert выше)
	SCB_CleanDCache_by_Addr((uint32_t*)color_p, wh_); // DMA читает RAM: выгрузить из кэша только отрисованный кусок
	lcd_dma_busy = true;
	Send_DMA_Data8(color_p, wh_);
}

static void my_input_read(lv_indev_t* /*indev*/, lv_indev_data_t* data) { // имя параметра перекрывало глобальный indev
	TouchPoints_HandleTypeDef TP = FT6336_GetTouchPoint(); // TD_STATUS и первая точка — одним чтением
	if (TP.count > 0) {
		data->point.x = TP.point1_x;
		data->point.y = TP.point1_y;
		data->state = LV_INDEV_STATE_PRESSED;
	}
	else {
		data->state = LV_INDEV_STATE_RELEASED;
	}
}

static void disp_create_and_touch_start() {

	disp = lv_display_create(320, 480);
	lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB888);
	lv_display_set_flush_cb(disp, my_flush_cb);
	lv_display_set_buffers(disp, buf_1, buf_2, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

	// LL_TIM_EnableIT_UPDATE(TIM6); // для TOUCH — не используется: TIM6 не инициализирован и не тактируется, обработчика нет (тач опрашивает LVGL)
	indev = lv_indev_create();
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, my_input_read);
}

void DMA2_Stream1_TransferComplete() { // DMA дисплея
	// Проверка флага Transfer Complete
	if (LL_DMA_IsActiveFlag_TC1(DMA2)) {
		LL_DMA_ClearFlag_TC1(DMA2);
	}

	// Проверка флага Transfer Error
	if (LL_DMA_IsActiveFlag_TE1(DMA2)) {
		LL_DMA_ClearFlag_TE1(DMA2);
		lcd_dma_errors = lcd_dma_errors + 1; // поток остановлен аппаратно; flush_ready ниже — LVGL не зависнет
	}

	// Проверка флага Half Transfer (если нужно)
	if (LL_DMA_IsActiveFlag_HT1(DMA2)) {
		LL_DMA_ClearFlag_HT1(DMA2);
	}

	// TC = последний байт попал в FIFO SPI, а не ушёл в линию. Без ожидания следующий
	// flush_cb опустит DC, и хвост FIFO (до 16 байт, ~1.4 мкс) дисплей примет как команды.
	while (!LL_SPI_IsActiveFlag_TXC(SPI3)) {
	};

	lcd_dma_busy = false;
	lv_display_flush_ready(disp);
}

// --------------------------------------------------------------- общее

// цвета — те же, что в проекте EEZ
const uint32_t col_text = 0xfafafa;
const uint32_t col_muted = 0x8b9098;
const uint32_t col_green = 0x2fc25b;
const uint32_t col_red = 0xe8692f;
const uint32_t col_amber = 0xeeab00;
const uint32_t col_danger = 0xff6b6b;
const uint32_t col_boot2 = 0xa98bf0;
const uint32_t col_none = 0x4a4e55;
const uint32_t col_bar = 0x3a3d42;
const uint32_t col_live = 0x0dcaf3;

static const char* const note_names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

// Сеттеры меняют объект, только если значение другое: иначе LVGL перерисует его зря
static void set_text(lv_obj_t* obj, const char* text) {
	if (strcmp(lv_label_get_text(obj), text) != 0) {
		lv_label_set_text(obj, text);
	}
}

static void set_textf(lv_obj_t* obj, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
static void set_textf(lv_obj_t* obj, const char* fmt, ...) {
	char buf[128];
	va_list va;
	va_start(va, fmt);
	lv_vsnprintf(buf, sizeof(buf), fmt, va);
	va_end(va);
	set_text(obj, buf);
}

static void set_hidden(lv_obj_t* obj, const bool hidden) {
	if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) != hidden) {
		lv_obj_set_flag(obj, LV_OBJ_FLAG_HIDDEN, hidden);
	}
}

static void set_color(lv_obj_t* obj, const uint32_t rgb) {
	const lv_color_t c = lv_color_hex(rgb);
	if (!lv_color_eq(lv_obj_get_style_text_color(obj, LV_PART_MAIN), c)) {
		lv_obj_set_style_text_color(obj, c, LV_PART_MAIN);
	}
}

static int user_data(lv_event_t* e) {
	return (int)(intptr_t)lv_event_get_user_data(e);
}

// мкс → «2.1 мс»
static void ms_text(char* buf, const uint32_t size, const float t_us) {
	const int tenths = (int)(t_us / 100.0f + 0.5f);
	lv_snprintf(buf, size, "%d.%d мс", tenths / 10, tenths % 10);
}

// мкс → «0.35 мс», для ползунков с шагом 10 мкс
static void ms2_text(char* buf, const uint32_t size, const int32_t t_us) {
	const int32_t hundredths = LV_ABS(t_us) / 10;
	lv_snprintf(buf, size, "%s%d.%02d мс", t_us < 0 ? "-" : "", (int)(hundredths / 100), (int)(hundredths % 100));
}

static uint32_t errors_total() {
	return noise_count + midi_lost + g4_ring_lost + lcd_dma_errors + ft6336_errors;
}
static uint32_t errors_seen = 0; // errors_total() при последнем открытии журнала

// «23 в piano · 1 в boot 1» — по состоянию каждой платы
static void chips_summary(char* buf, const uint32_t size) {
	uint32_t n[4] = {};
	for (const auto& chip : vChips) {
		++n[(uint32_t)chip.st & 3];
	}
	if (vChips.empty()) {
		lv_snprintf(buf, size, "ни одна плата не ответила");
		return;
	}
	static const char* const names[4] = { "без состояния", "boot 1", "piano", "boot 2" };
	static const uint32_t order[4] = { 2, 1, 3, 0 };
	uint32_t len = 0;
	for (const uint32_t s : order) {
		if (n[s] && len < size) {
			len += lv_snprintf(buf + len, size - len, "%s%" LV_PRIu32 " в %s", len ? " · " : "", n[s], names[s]);
		}
	}
}

// --------------------------------------------------------------- панель подтверждения

typedef void (*gui_fn)();
static lv_obj_t* confirm_layer = nullptr;
static gui_fn confirm_ok = nullptr;
static gui_fn confirm_alt = nullptr;

// user_data: 0 — отмена (мимо панели), 1 — действие, 2 — второе действие
static void confirm_clicked(lv_event_t* e) {
	const int k = user_data(e);
	const gui_fn fn = k == 1 ? confirm_ok : k == 2 ? confirm_alt : nullptr;
	lv_obj_t* layer = confirm_layer;
	confirm_layer = nullptr;
	if (fn) {
		lv_obj_add_flag(layer, LV_OBJ_FLAG_HIDDEN);
		lv_refr_now(NULL); // действия бывают долгими (шина, flash) — экран без панели до их начала
		fn();
	}
	lv_obj_delete_async(layer); // удалять объект, чьё событие ещё идёт, — только так
}

static lv_obj_t* confirm_button(lv_obj_t* card, const int32_t x, const char* text, const int k, const uint32_t bg, const uint32_t color) {
	lv_obj_t* b = lv_button_create(card);
	lv_obj_set_pos(b, x, 132);
	lv_obj_set_size(b, 120, 48);
	lv_obj_set_style_radius(b, 8, LV_PART_MAIN);
	lv_obj_set_style_bg_color(b, lv_color_hex(bg), LV_PART_MAIN);
	lv_obj_add_event_cb(b, confirm_clicked, LV_EVENT_CLICKED, (void*)(intptr_t)k);
	lv_obj_t* l = lv_label_create(b);
	lv_obj_set_style_text_color(l, lv_color_hex(color), LV_PART_MAIN);
	lv_label_set_text(l, text);
	lv_obj_center(l);
	return b;
}

// Вопрос перед необратимым. Справа — действие (глагол), слева — «Отмена» или второе действие; мимо панели — отмена
static void confirm(const char* title, const char* text, const char* ok_text, gui_fn ok, const bool danger,
	const char* alt_text = nullptr, gui_fn alt = nullptr) {
	if (confirm_layer) {
		return;
	}
	confirm_ok = ok;
	confirm_alt = alt;

	lv_obj_t* layer = lv_obj_create(lv_layer_top());
	confirm_layer = layer;
	lv_obj_remove_style_all(layer);
	lv_obj_set_size(layer, 320, 480);
	lv_obj_set_style_bg_color(layer, lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(layer, LV_OPA_60, LV_PART_MAIN);
	lv_obj_add_flag(layer, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(layer, confirm_clicked, LV_EVENT_CLICKED, (void*)0);

	lv_obj_t* card = lv_obj_create(layer);
	lv_obj_set_pos(card, 20, 140);
	lv_obj_set_size(card, 280, 196);
	lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
	lv_obj_set_style_radius(card, 10, LV_PART_MAIN);
	lv_obj_set_style_bg_color(card, lv_color_hex(0x24272b), LV_PART_MAIN);

	lv_obj_t* l = lv_label_create(card);
	lv_obj_set_pos(l, 16, 16);
	lv_obj_set_width(l, 248);
	lv_label_set_text(l, title);

	l = lv_label_create(card);
	lv_obj_set_pos(l, 16, 46);
	lv_obj_set_width(l, 248);
	lv_obj_set_style_text_color(l, lv_color_hex(col_muted), LV_PART_MAIN);
	lv_label_set_text(l, text);

	confirm_button(card, 16, alt_text ? alt_text : "Отмена", alt_text ? 2 : 0, 0x343638, col_text);
	confirm_button(card, 144, ok_text, 1, danger ? 0x7a1f22 : 0x343638, danger ? 0xffffff : col_amber);
}

// --------------------------------------------------------------- главный экран

// график кривой — по логарифму t, от curve_t_min до curve_t_max
const uint32_t curve_points_count = 100;
const float curve_t_min = 200.0f;   // мкс, = user_t_fast_min
const float curve_t_max = 30000.0f; // мкс, = user_t_slow_max
static int32_t main_curve_points[curve_points_count] = {};
static bool show_last_note = false; // строка «последняя нота» (включается на экране «Сервис»)
static uint32_t last_note_shown = 0;
static int32_t main_tile_grow = 0; // на сколько плитка кривой длиннее без строки «последняя нота» — до её низа

// В EEZ строка видна, а плитка короткая. Без строки плитка и её график растягиваются вниз на её место
static void main_last_note_layout() {
	if (lv_obj_has_flag(objects.main_last_note, LV_OBJ_FLAG_HIDDEN) != show_last_note) {
		return;
	}
	lv_obj_set_flag(objects.main_last_note, LV_OBJ_FLAG_HIDDEN, !show_last_note);
	const int32_t d = show_last_note ? -main_tile_grow : main_tile_grow;
	lv_obj_set_height(objects.main_curve_tile, lv_obj_get_style_height(objects.main_curve_tile, LV_PART_MAIN) + d);
	lv_obj_set_height(objects.main_curve_chart, lv_obj_get_style_height(objects.main_curve_chart, LV_PART_MAIN) + d);
}

static void main_curve_update() {
	char name[24];
	lv_roller_get_selected_str(objects.curve_roller, name, sizeof(name));
	set_text(objects.main_curve_name, name);
	set_textf(objects.main_curve_modes, "OFF %s · LIFT %s", curve_off_selected ? "log" : "lin", curve_lift_selected ? "log" : "lin");
	for (uint32_t i = 0; i < curve_points_count; ++i) {
		const float t = curve_t_min * powf(curve_t_max / curve_t_min, (float)i / (curve_points_count - 1));
		main_curve_points[i] = (int32_t)LV_CLAMP(1.0f, on_vel(curve_on_selected, t), 127.0f);
	}
	lv_chart_refresh(objects.main_curve_chart);
}

static void main_status_update() {
	const uint32_t n = vChips.size();
	bool all_piano = true;
	for (const auto& chip : vChips) {
		all_piano = all_piano && chip.st == chip_states::piano;
	}
	if (n) {
		set_textf(objects.main_status_chips, "G4: %" LV_PRIu32, n);
	}
	else {
		set_text(objects.main_status_chips, "G4: нет");
	}
	set_color(objects.main_status_chips, n == 0 ? col_danger : all_piano ? col_green : col_amber);
	set_color(objects.main_status_usb, host_reads ? col_green : col_muted);
	set_hidden(objects.main_status_calib, calib_dirty == 0);

	const uint32_t warn = errors_total() - errors_seen;
	set_hidden(objects.main_status_warn, warn == 0);
	if (warn) {
		set_textf(objects.main_status_warn_label, "▲ %" LV_PRIu32, warn);
	}

	main_last_note_layout();
	if (show_last_note && last_note_seq != last_note_shown) {
		last_note_shown = last_note_seq;
		const uint32_t note = last_note;
		const uint32_t vel = last_note_vel;
		const uint32_t t = last_note_t_us / 100; // десятые мс
		set_textf(objects.main_last_note_label, "%s%d  ·  vel %" LV_PRIu32 "  ·  %" LV_PRIu32 ".%" LV_PRIu32 " мс",
			note_names[note % 12], (int)(note / 12) - 1, vel, t / 10, t % 10);
		lv_bar_set_value(objects.main_last_note_bar, vel, LV_ANIM_OFF);
	}
}

// --------------------------------------------------------------- кривые

static uint32_t curve_tab = 0; // 0 — ON, 1 — OFF, 2 — LIFT
static int32_t curve_points[curve_points_count] = {};

// График, ползунки и кнопки — для вкладки и кривой, выбранной в роллере: user — все три ползунка и «Сохранить»,
// user lin и user log — t_fast, t_slow и «Сброс», остальные ON не правятся. OFF и LIFT — lin или log
static void curve_screen_update() {
	const uint32_t c = curve_on_selected;
	const bool on = curve_tab == 0;
	const bool user = on && c == curve_user;
	const bool lin = on && c == curve_user_lin;
	const bool log = on && c == curve_user_log;
	float t_fast = user_curve.t_fast;
	float t_slow = user_curve.t_slow;
	if (lin) {
		t_fast = user_lin_t_fast;
		t_slow = user_lin_t_slow;
	}
	if (log) {
		t_fast = user_log_t_fast;
		t_slow = user_log_t_slow;
	}

	lv_obj_set_state(objects.curve_tab_on, LV_STATE_CHECKED, curve_tab == 0);
	lv_obj_set_state(objects.curve_tab_off, LV_STATE_CHECKED, curve_tab == 1);
	lv_obj_set_state(objects.curve_tab_lift, LV_STATE_CHECKED, curve_tab == 2);
	set_hidden(objects.curve_roller, !on);
	set_hidden(objects.curve_mode_info, on);
	set_hidden(objects.curve_mode_panel, on);
	set_hidden(objects.curve_sliders, !(user || lin || log));
	set_hidden(objects.curve_row_x0, !user);
	set_hidden(objects.curve_fixed, !on || user || lin || log);
	set_hidden(objects.curve_save, !user);
	set_hidden(objects.curve_reset, !(lin || log));

	lv_slider_set_value(objects.curve_s_fast, (int32_t)t_fast, LV_ANIM_OFF); // событие не шлёт
	lv_slider_set_value(objects.curve_s_slow, (int32_t)t_slow, LV_ANIM_OFF);
	lv_slider_set_value(objects.curve_s_x0, (int32_t)user_curve.x0, LV_ANIM_OFF);
	char v[16];
	ms2_text(v, sizeof(v), (int32_t)t_fast);
	set_text(objects.curve_v_fast, v);
	ms_text(v, sizeof(v), t_slow); // шаг 100 мкс — десятых хватает
	set_text(objects.curve_v_slow, v);
	ms2_text(v, sizeof(v), (int32_t)user_curve.x0);
	set_text(objects.curve_v_x0, v);

	// OFF и LIFT: от половины t_fast до полутора t_slow — видно, где кривая упирается в 127 и в 1
	float t_min = curve_t_min;
	float t_max = curve_t_max;
	float mode_fast = off_t_fast;
	float mode_slow = off_t_slow;
	bool mode_log = curve_off_selected;
	if (curve_tab == 2) {
		mode_fast = lift_t_fast;
		mode_slow = lift_t_slow;
		mode_log = curve_lift_selected;
	}
	if (!on) {
		t_min = mode_fast / 2.0f;
		t_max = mode_slow * 1.5f;
	}
	for (uint32_t i = 0; i < curve_points_count; ++i) {
		const float t = t_min * powf(t_max / t_min, (float)i / (curve_points_count - 1));
		const float v = on ? on_vel(c, t) : speed_vel(t, mode_fast, mode_slow, mode_log);
		curve_points[i] = (int32_t)LV_CLAMP(1.0f, v, 127.0f);
	}
	lv_chart_refresh(objects.curve_chart);

	char a[16];
	char b[16];
	ms_text(a, sizeof(a), t_min);
	set_text(objects.curve_axis_min, a);
	ms_text(a, sizeof(a), t_max);
	set_text(objects.curve_axis_max, a);
	if (on) {
		set_textf(objects.curve_info, "2 мс → %d", (int)LV_CLAMP(1.0f, on_vel(c, 2000.0f), 127.0f));
	}
	else {
		set_text(objects.curve_info, "t, лог. шкала");
		ms_text(a, sizeof(a), mode_fast);
		ms_text(b, sizeof(b), mode_slow);
		set_textf(objects.curve_mode_info, "127 при\n%s\n\n1 при\n%s", a, b);
		lv_obj_set_state(objects.curve_mode_lin, LV_STATE_CHECKED, !mode_log);
		lv_obj_set_state(objects.curve_mode_log, LV_STATE_CHECKED, mode_log);
	}
}

// ползунки правят кривую, выбранную в роллере (скрытые у неё не используются)
static void curve_apply_sliders() {
	const float t_fast = (float)lv_slider_get_value(objects.curve_s_fast);
	const float t_slow = (float)lv_slider_get_value(objects.curve_s_slow);
	if (curve_on_selected == curve_user) {
		user_curve.t_fast = t_fast;
		user_curve.t_slow = t_slow;
		user_curve.x0 = (float)lv_slider_get_value(objects.curve_s_x0);
		user_curve_set();
		set_text(objects.curve_save_label, "Сохранить");
	}
	else if (curve_on_selected == curve_user_lin) {
		user_lin_t_fast = t_fast;
		user_lin_t_slow = t_slow;
	}
	else if (curve_on_selected == curve_user_log) {
		user_log_t_fast = t_fast;
		user_log_t_slow = t_slow;
	}
	curve_screen_update();
	main_curve_update();
}

// --------------------------------------------------------------- калибровка

const uint32_t calib_on_base = 7;     // адрес компаратора 0 чипа 1 — первая точка графика ON
const uint32_t calib_on_points = 91;  // 13 чипов ON × 7
const uint32_t calib_off_points = 70; // 10 чипов OFF × 7, от buffer_division
static lv_chart_series_t* ser_green;
static lv_chart_series_t* ser_red;
static lv_chart_series_t* ser_blue;
static lv_chart_series_t* ser_dac;
static lv_chart_cursor_t* cursor_vert;
static lv_chart_cursor_t* cursor_hor;
static lv_obj_t* scale_r;
static lv_obj_t* scale_g;
static lv_obj_t* scale_live;
static lv_obj_t* scale_r_label;
static lv_obj_t* scale_g_label;

static uint32_t calib_base() {
	return cur_disp == current_display::on ? calib_on_base : buffer_division;
}

static void calib_chart_config() {
	lv_obj_t* ob = objects.calib_chart;
	ser_green = lv_chart_add_series(ob, lv_color_hex(0x00ff00), LV_CHART_AXIS_PRIMARY_Y);
	ser_red = lv_chart_add_series(ob, lv_color_hex(0xff0000), LV_CHART_AXIS_PRIMARY_Y);
	ser_blue = lv_chart_add_series(ob, lv_color_hex(0x0dcaf3), LV_CHART_AXIS_PRIMARY_Y); // LV_COLOR_MAKE(0xE9, 0x1E, 0x63)
	ser_dac = lv_chart_add_series(ob, lv_color_hex(0x757575), LV_CHART_AXIS_PRIMARY_Y);
	cursor_vert = lv_chart_add_cursor(ob, lv_color_hex(0x808080), LV_DIR_VER);
	cursor_hor = lv_chart_add_cursor(ob, lv_color_hex(0x808080), LV_DIR_HOR);
	lv_obj_set_style_line_width(ob, 1, LV_PART_CURSOR); // толщина курсора
	lv_obj_set_style_line_width(ob, 0, LV_PART_ITEMS);  // толщина линий между точками на графике
	lv_obj_set_style_size(ob, 2, 3, LV_PART_INDICATOR); // размер точек на графике
}

// серии и масштаб — для молоточков или демпферов (cur_disp)
static void calib_chart_setup() {
	const bool on = cur_disp == current_display::on;
	const uint32_t base = calib_base();
	lv_obj_t* ob = objects.calib_chart;
	lv_chart_set_point_count(ob, on ? calib_on_points : calib_off_points);
	lv_chart_set_series_ext_y_array(ob, ser_green, &buffer_green[base]);
	lv_chart_set_series_ext_y_array(ob, ser_red, &buffer_red[base]);
	lv_chart_set_series_ext_y_array(ob, ser_blue, &buffer_blue_calib[base]);
	lv_chart_set_series_ext_y_array(ob, ser_dac, &buffer_dac[base]);
	const int32_t lo = on ? graph_min_max.on_min : graph_min_max.off_max;
	const int32_t hi = on ? graph_min_max.on_max : graph_min_max.off_min;
	lv_chart_set_axis_range(ob, LV_CHART_AXIS_PRIMARY_Y, lo, hi);
	lv_chart_set_axis_range(ob, LV_CHART_AXIS_SECONDARY_Y, lo, hi);
	lv_obj_set_state(objects.calib_tab_on, LV_STATE_CHECKED, on);
	lv_obj_set_state(objects.calib_tab_off, LV_STATE_CHECKED, !on);
}

static lv_obj_t* scale_rect(const int32_t x, const int32_t y, const int32_t w, const int32_t h, const uint32_t color) {
	lv_obj_t* o = lv_obj_create(objects.calib_scale);
	lv_obj_remove_style_all(o);
	lv_obj_remove_flag(o, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
	lv_obj_set_pos(o, x, y);
	lv_obj_set_size(o, w, h);
	lv_obj_set_style_bg_color(o, lv_color_hex(color), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN);
	return o;
}

static lv_obj_t* scale_label(const uint32_t color) {
	lv_obj_t* l = lv_label_create(objects.calib_scale);
	lv_obj_set_width(l, 56);
	lv_obj_set_style_text_font(l, &ui_font_ru_12, LV_PART_MAIN);
	lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_style_text_color(l, lv_color_hex(color), LV_PART_MAIN);
	lv_label_set_text(l, "");
	return l;
}

// шкала порогов клавиши под курсором: 0..4095 ЦАП слева направо, подписи над и под ней
const int32_t scale_label_top = 0;
const int32_t scale_label_bottom = 33;

static void calib_scale_create() {
	lv_obj_t* bar = scale_rect(0, 21, 280, 6, col_bar);
	lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
	scale_r = scale_rect(0, 16, 3, 16, col_red);
	scale_g = scale_rect(0, 16, 3, 16, col_green);
	scale_live = scale_rect(0, 16, 3, 16, col_live);
	scale_r_label = scale_label(col_red);
	scale_g_label = scale_label(col_green);
}

static void scale_mark(lv_obj_t* mark, lv_obj_t* label, const int32_t v, const char* name, const int32_t label_y) {
	const int32_t x = LV_CLAMP(0, v, 4095) * 277 / 4095;
	lv_obj_set_x(mark, x);
	if (label) {
		lv_obj_set_pos(label, LV_CLAMP(0, x - 28, 280 - 56), label_y);
		set_textf(label, "%s %d", name, (int)v);
	}
}

static void calib_refresh() {
	const bool on = cur_disp == current_display::on;
	if (calib_dirty) {
		set_textf(objects.calib_save_label, "Сохр. %" LV_PRIu32, calib_dirty);
	}
	else {
		set_text(objects.calib_save_label, "Сохр.");
	}
	lv_obj_set_state(objects.calib_follow, LV_STATE_CHECKED, calib_follow);

	const comparator& comp = searcher_addr_in_cursor(cursor);
	if (!comp.is_active) {
		set_text(objects.calib_key, "Нет плат");
		set_text(objects.calib_key_info, "—");
		set_text(objects.calib_live, "—");
		set_text(objects.calib_neighbors, "");
		set_text(objects.calib_margin, "");
		set_text(objects.calib_g_change, "—");
		set_text(objects.calib_r_change, "—");
		return;
	}

	// точка графика — по адресу компаратора, а не по номеру курсора: без одной платы номера сдвинуты
	const uint32_t addr = comp.address;
	const uint32_t idx = addr - calib_base();
	lv_chart_set_cursor_point(objects.calib_chart, cursor_vert, ser_blue, idx);
	lv_chart_set_cursor_point(objects.calib_chart, cursor_hor, ser_blue, idx);
	lv_chart_refresh(objects.calib_chart);

	// нота — как в g4_to_midi: G4 шлёт адрес OFF на 2 меньше, чем он лежит в буферах H7
	const uint32_t rx = on ? addr : addr - 2;
	const int note = (int)rx + noteAdder[rx];
	set_textf(objects.calib_key, "Клавиша %d", note - 20);
	set_textf(objects.calib_key_info, "%s%d · чип %u · комп. %u", note_names[note % 12], note / 12 - 1,
		(unsigned)comp.number_chip, (unsigned)comp.number_comparator);

	const int32_t live = buffer_blue_calib[addr];
	const int32_t g = buffer_green[addr];
	const int32_t r = buffer_red[addr];
	set_textf(objects.calib_live, "%d", (int)live);
	set_textf(objects.calib_neighbors, "← %d · %d →", (int)buffer_blue_calib[addr - 1], (int)buffer_blue_calib[addr + 1]);
	// запас между порогами: молоточкам нужно G > R, демпферам G < R; уже 130 кодов —
	// гистерезис 70 мВ (~87 кодов) плюс его разброс и шум обучения (калибровка.md п. 7)
	const int32_t margin = on ? g - r : r - g;
	set_textf(objects.calib_margin, on ? "G − R  %d" : "R − G  %d", (int)margin);
	set_color(objects.calib_margin, margin >= 130 ? col_muted : col_amber);
	set_textf(objects.calib_g_change, "%d → %d", (int)g, (int)live);
	set_textf(objects.calib_r_change, "%d → %d", (int)r, (int)live);

	// R и G по разные стороны шкалы — близкие значения не наползают: у молоточков G снизу, у демпферов сверху
	scale_mark(scale_r, scale_r_label, r, "R", on ? scale_label_top : scale_label_bottom);
	scale_mark(scale_g, scale_g_label, g, "G", on ? scale_label_bottom : scale_label_top);
	scale_mark(scale_live, nullptr, live, "", 0);
}

static void calib_exit() {
	bus_resume();
	loadScreen(SCREEN_ID_D_MAIN);
}

static void calib_save_and_exit() {
	calib_save();
	calib_exit();
}

// --------------------------------------------------------------- сервис

static lv_obj_t* chip_cells[allChipCount] = {};

static void chip_grid_create() {
	for (uint32_t n = 1; n < allChipCount; ++n) {
		const uint32_t row = n < on_off_division ? 0 : 1;
		const uint32_t col = row ? n - on_off_division : n - 1;
		lv_obj_t* o = lv_obj_create(objects.service_chip_grid);
		lv_obj_remove_style_all(o);
		lv_obj_remove_flag(o, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
		lv_obj_set_pos(o, col * 19, row * 24);
		lv_obj_set_size(o, 17, 17);
		lv_obj_set_style_radius(o, 4, LV_PART_MAIN);
		lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN);
		chip_cells[n] = o;
	}
}

static void service_refresh() {
	char buf[96];
	chips_summary(buf, sizeof(buf));
	set_text(objects.service_chips_summary, buf);
	for (uint32_t n = 1; n < allChipCount; ++n) {
		uint32_t color = col_none;
		for (const auto& chip : vChips) {
			if (chip.number_chip == n) {
				color = chip.st == chip_states::piano ? col_green : chip.st == chip_states::boot1 ? col_amber
					: chip.st == chip_states::boot2 ? col_boot2 : col_none;
			}
		}
		lv_obj_set_style_bg_color(chip_cells[n], lv_color_hex(color), LV_PART_MAIN);
	}
	set_text(objects.service_last_note_state, show_last_note ? "вкл" : "выкл");
	set_color(objects.service_last_note_state, show_last_note ? col_green : col_muted);
}

static void service_warn_update() {
	const uint32_t warn = errors_total() - errors_seen;
	set_hidden(objects.service_warn, warn == 0);
	if (warn) {
		set_textf(objects.service_warn, "▲ %" LV_PRIu32, warn);
	}
}

static void jump_to_dfu() {
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

static void calib_restore_and_refresh() {
	calib_restore();
	service_refresh();
}

// --------------------------------------------------------------- прошивка G4

static bool flash_boot_open = false; // блок «Загрузчики» раскрыт
const uint32_t flash_log_lines = 13;  // строк журнала под шагами

static void flash_refresh() {
	char buf[96];
	chips_summary(buf, sizeof(buf));
	set_text(objects.flash_state, buf);
	lv_obj_set_state(objects.flash_step1, LV_STATE_DISABLED, chip_state != chip_states::piano);
	lv_obj_set_state(objects.flash_step2, LV_STATE_DISABLED, chip_state != chip_states::boot1);
	lv_obj_set_state(objects.flash_step3, LV_STATE_DISABLED, chip_state != chip_states::boot1);
	set_text(objects.flash_boot_toggle_label, flash_boot_open ? "▼  Загрузчики" : "▶  Загрузчики");
	set_hidden(objects.flash_boot_panel, !flash_boot_open);
	set_hidden(objects.flash_log, flash_boot_open);
}

static uint32_t flash_log_counter = UINT32_MAX;

// хвост журнала: последние flash_log_lines строк
static void flash_log_update() {
	if (debug_counter == flash_log_counter) {
		return;
	}
	flash_log_counter = debug_counter;
	size_t pos = debugg.size();
	for (uint32_t n = 0; n < flash_log_lines && pos > 0; ++n) {
		pos = debugg.rfind('\n', pos - 1);
		if (pos == std::string::npos) {
			pos = 0;
			break;
		}
	}
	set_text(objects.flash_log, debugg.c_str() + (pos ? pos + 1 : 0));
}

static void flash_after() {
	flash_log_counter = UINT32_MAX;
	flash_refresh();
}

static void flash_step1() {
	reset_main_to_bootloader();
	init_chips();
	flash_after();
}

static void flash_step2() {
	data_from_H7_to_g4(ADDRESS_H7_MAIN_FIRMWARE_FOR_G4, COUNT_PAGE_FOR_FIRMWARE_G4, ADDRESS_G4_MAIN_FIRMWARE);
	flash_after();
}

// как при старте H7 с платами в загрузчике (init): прыжок, пауза на запуск, опрос
static void flash_step3() {
	jump_g4s_to_adress(ADDRESS_G4_MAIN_FIRMWARE, chip_states::piano);
	pause(50000);
	init_chips();
	flash_after();
}

static void flash_boot0() {
	data_from_H7_to_g4(ADDRESS_H7_MAIN_FIRMWARE_FOR_G4, COUNT_PAGE_G4_FOR_BOOTLOADER_v2, ADDRESS_G4_BOOTLOADER_v2);
	flash_after();
}

static void flash_boot1() {
	jump_g4s_to_adress(ADDRESS_G4_BOOTLOADER_v2, chip_states::boot2);
	init_chips();
	flash_after();
}

static void flash_boot2() {
	data_from_H7_to_g4(ADDRESS_H7_MAIN_FIRMWARE_FOR_G4, COUNT_PAGE_G4_FOR_BOOTLOADER, ADDRESS_G4_BOOTLOADER);
	flash_after();
}

static void flash_boot3() {
	reset_bootloaders();
	init_chips();
	flash_after();
}

// --------------------------------------------------------------- журнал

static enum ScreensEnum log_return = SCREEN_ID_D_SERVICE; // откуда открыт журнал — туда «‹»
static uint32_t log_counter = UINT32_MAX;

static void log_update(const bool force) {
	set_textf(objects.log_counters, "USART noise %" LV_PRIu32 " · MIDI lost %" LV_PRIu32 " · G4 ring %" LV_PRIu32
		"\nLCD DMA %" LV_PRIu32 " · Touch I2C %" LV_PRIu32,
		(uint32_t)noise_count, (uint32_t)midi_lost, (uint32_t)g4_ring_lost, (uint32_t)lcd_dma_errors, ft6336_errors);
	if (force || debug_counter != log_counter) {
		log_counter = debug_counter;
		set_text(objects.log_text, debugg.c_str());
	}
}

// --------------------------------------------------------------- init и основной цикл

static int32_t* curve_chart_config(lv_obj_t* chart, int32_t* points) {
	lv_chart_series_t* ser = lv_chart_add_series(chart, lv_color_hex(0x0dcaf3), LV_CHART_AXIS_PRIMARY_Y);
	lv_chart_set_series_ext_y_array(chart, ser, points);
	lv_chart_set_axis_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 127);
	lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR); // без точек — только линия
	return points;
}

void gui_init() {
	init_LCD_and_touch();
	lv_init();
	disp_create_and_touch_start();
	ui_init();

	calib_chart_config();
	calib_scale_create();
	curve_chart_config(objects.curve_chart, curve_points);
	curve_chart_config(objects.main_curve_chart, main_curve_points);
	chip_grid_create();

	lv_obj_t* note = objects.main_last_note;
	lv_obj_t* tile = objects.main_curve_tile;
	main_tile_grow = lv_obj_get_style_y(note, LV_PART_MAIN) + lv_obj_get_style_height(note, LV_PART_MAIN)
		- lv_obj_get_style_y(tile, LV_PART_MAIN) - lv_obj_get_style_height(tile, LV_PART_MAIN);
	main_last_note_layout();

	curve_on_selected = lv_roller_get_selected(objects.curve_roller);
	curve_screen_update();
	main_curve_update();
}

void gui_tick() {
	static uint32_t last = 0;
	if (HAL_GetTick() - last < 20) { // 50 раз в секунду хватает глазу; главный цикл — для шины и LVGL
		return;
	}
	last = HAL_GetTick();

	lv_obj_t* scr = lv_screen_active();
	if (scr == objects.d_main) {
		main_status_update();
	}
	else if (scr == objects.d_calib) {
		if (calib_data_new) {
			calib_data_new = false;
			calib_refresh();
		}
	}
	else if (scr == objects.d_service) {
		service_warn_update();
	}
	else if (scr == objects.d_flash) {
		flash_log_update();
	}
	else if (scr == objects.d_log) {
		log_update(false);
	}
}

// --------------------------------------------------------------- действия (EEZ: ui/actions.h)
extern "C" {

	// переходы

	void action_to_main(lv_event_t* e) {
		loadScreen(SCREEN_ID_D_MAIN);
	}

	void action_to_curve(lv_event_t* e) {
		curve_screen_update();
		loadScreen(SCREEN_ID_D_CURVE);
	}

	void action_to_calib(lv_event_t* e) {
		calib_enter(typeAction::on);
		calib_chart_setup();
		calib_refresh();
		loadScreen(SCREEN_ID_D_CALIB);
	}

	void action_to_service(lv_event_t* e) {
		service_refresh();
		service_warn_update();
		loadScreen(SCREEN_ID_D_SERVICE);
	}

	void action_to_flash(lv_event_t* e) {
		bus_stop();
		flash_log_counter = UINT32_MAX;
		flash_refresh();
		flash_log_update();
		loadScreen(SCREEN_ID_D_FLASH);
	}

	void action_to_log(lv_event_t* e) {
		log_return = lv_screen_active() == objects.d_main ? SCREEN_ID_D_MAIN : SCREEN_ID_D_SERVICE;
		errors_seen = errors_total();
		log_update(true);
		loadScreen(SCREEN_ID_D_LOG);
	}

	void action_log_back(lv_event_t* e) {
		if (log_return == SCREEN_ID_D_SERVICE) {
			service_warn_update();
		}
		loadScreen(log_return);
	}

	void action_power_off(lv_event_t* e) {
		request_power_off();
	}

	// кривые

	void action_curve_tab(lv_event_t* e) {
		curve_tab = user_data(e);
		curve_screen_update();
	}

	void action_curve_roller_changed(lv_event_t* e) {
		curve_on_selected = lv_roller_get_selected(objects.curve_roller);
		curve_screen_update();
		main_curve_update();
	}

	void action_curve_slider_changed(lv_event_t* e) {
		curve_apply_sliders();
	}

	// «−» и «+» у ползунков: user_data — номер ползунка × 2, +1 у «+»
	void action_curve_step(lv_event_t* e) {
		static lv_obj_t* const* const sliders[3] = { &objects.curve_s_fast, &objects.curve_s_slow, &objects.curve_s_x0 };
		static const int32_t steps[3] = { 10, 100, 10 }; // мкс
		const int k = user_data(e);
		lv_obj_t* s = *sliders[k / 2];
		const int32_t step = k % 2 ? steps[k / 2] : -steps[k / 2];
		lv_slider_set_value(s, lv_slider_get_value(s) + step, LV_ANIM_OFF);
		curve_apply_sliders();
	}

	void action_curve_mode(lv_event_t* e) {
		if (curve_tab == 1) {
			curve_off_selected = user_data(e);
		}
		else if (curve_tab == 2) {
			curve_lift_selected = user_data(e);
		}
		curve_screen_update();
		main_curve_update();
	}

	void action_curve_save(lv_event_t* e) {
		set_text(objects.curve_save_label, user_curve_save() ? "Сохранено" : "Ошибка flash");
	}

	// выбранную user lin или user log — к vel_t_fast / vel_t_slow, как при старте
	void action_curve_reset(lv_event_t* e) {
		if (curve_on_selected == curve_user_lin) {
			user_lin_t_fast = vel_t_fast;
			user_lin_t_slow = vel_t_slow;
		}
		else if (curve_on_selected == curve_user_log) {
			user_log_t_fast = vel_t_fast;
			user_log_t_slow = vel_t_slow;
		}
		curve_screen_update();
		main_curve_update();
	}

	// калибровка

	void action_calib_back(lv_event_t* e) {
		if (!calib_dirty) {
			calib_exit();
			return;
		}
		static char text[96];
		lv_snprintf(text, sizeof(text), "Задано порогов: %" LV_PRIu32 ". Без сохранения они пропадут после выключения.", calib_dirty);
		confirm("Сохранить калибровку?", text, "Сохранить", calib_save_and_exit, false, "Не сохранять", calib_exit);
	}

	void action_calib_tab(lv_event_t* e) {
		const typeAction t = user_data(e) ? typeAction::off : typeAction::on;
		if ((t == typeAction::on) == (cur_disp == current_display::on)) {
			return;
		}
		calib_leave();
		calib_enter(t);
		calib_chart_setup();
		calib_refresh();
	}

	void action_calib_save(lv_event_t* e) {
		calib_save();
		calib_refresh();
	}

	// касание графика выбирает клавишу этой точки
	void action_calib_chart_pressed(lv_event_t* e) {
		const uint32_t idx = lv_chart_get_pressed_point(objects.calib_chart);
		if (idx == LV_CHART_POINT_NONE) {
			return;
		}
		const uint32_t addr = calib_base() + idx;
		const auto& map = cur_disp == current_display::on ? mCursor_to_comparator_on : mCursor_to_comparator_off;
		for (const auto& [c, comp] : map) {
			if (comp.address == addr && c != cursor) {
				cursor = c;
				calib_refresh();
				return;
			}
		}
	}

	// «7, ‹, ›, 7»: на плату (7 клавиш) или на клавишу
	void action_calib_step(lv_event_t* e) {
		static const int32_t delta[4] = { -7, -1, 1, 7 };
		calib_cursor_move(delta[user_data(e) & 3]);
		calib_refresh();
	}

	void action_calib_follow(lv_event_t* e) {
		calib_follow = !calib_follow;
		calib_refresh();
	}

	void action_calib_set(lv_event_t* e) {
		calib_set_threshold(user_data(e) ? dot::red : dot::green);
		calib_refresh();
	}

	// сервис

	void action_rescan(lv_event_t* e) {
		chips_rescan();
		service_refresh();
		flash_after();
	}

	void action_toggle_last_note(lv_event_t* e) {
		show_last_note = !show_last_note;
		service_refresh();
	}

	void action_restore_calibration(lv_event_t* e) {
		confirm("Восстановить калибровку?", "Пороги из памяти H7 заменят текущие, в том числе несохранённые.",
			"Восстановить", calib_restore_and_refresh, true);
	}

	void action_jump_to_dfu(lv_event_t* e) {
		confirm("Перейти в USB DFU?", "H7 перезагрузится в DFU-загрузчик. Пианино замолчит.", "Перейти", jump_to_dfu, true);
	}

	void action_reboot(lv_event_t* e) {
		confirm("Перезагрузить H7?", "Пианино замолчит, пока H7 запускается.", "Перезагрузить", to_sleep, true);
	}

	// прошивка G4

	void action_flash_back(lv_event_t* e) {
		bus_resume();
		service_refresh();
		loadScreen(SCREEN_ID_D_SERVICE);
	}

	void action_flash_step(lv_event_t* e) {
		switch (user_data(e)) {
		case 1:
			confirm("Перевести платы в загрузчик?", "Пианино замолчит до шага 3.", "Перевести", flash_step1, true);
			break;
		case 2:
			confirm("Записать прошивку в G4?", "Образ из памяти H7 запишется в каждую плату.", "Записать", flash_step2, true);
			break;
		case 3:
			confirm("Запустить piano?", "Платы перейдут из загрузчика в основную прошивку.", "Запустить", flash_step3, false);
			break;
		}
	}

	void action_flash_boot_toggle(lv_event_t* e) {
		flash_boot_open = !flash_boot_open;
		flash_after();
	}

	void action_flash_boot(lv_event_t* e) {
		switch (user_data(e)) {
		case 0:
			confirm("boot 1 → записать boot 2?", "Загрузчик 1 запишет загрузчик 2 во все платы.", "Записать", flash_boot0, true);
			break;
		case 1:
			confirm("boot 1 → перейти в boot 2?", "Платы перейдут в загрузчик 2.", "Перейти", flash_boot1, true);
			break;
		case 2:
			confirm("boot 2 → записать boot 1?", "Загрузчик 2 запишет загрузчик 1 во все платы. Ошибка здесь — только SWD.", "Записать", flash_boot2, true);
			break;
		case 3:
			confirm("Сбросить загрузчики?", "Платы в загрузчике перезапустятся в boot 1.", "Сбросить", flash_boot3, true);
			break;
		}
	}

	// журнал

	void action_clear_log(lv_event_t* e) {
		debugg_clear();
		log_update(true);
	}

}
