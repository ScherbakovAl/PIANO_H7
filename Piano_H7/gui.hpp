#pragma once

// Интерфейс на дисплее: порт LVGL (дисплей, тач), экраны из EEZ (ui/) и то, что в EEZ не задать — серии графиков,
// сетка плат, шкала порогов, панель подтверждения. Действия кнопок (action_* из ui/actions.h) — в gui.cpp

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

	extern lv_display_t* disp;

	void gui_init();  // дисплей, тач, LVGL, экраны
	void gui_tick();  // из основного цикла: строка статуса, журнал, экран калибровки — только при изменениях
	void DMA2_Stream1_TransferComplete();

#ifdef __cplusplus
}
#endif
