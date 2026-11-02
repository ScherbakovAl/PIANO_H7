# PIANO_H7 — Controller board firmware / Прошивка платы-контроллера

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.20569239.svg)](https://doi.org/10.5281/zenodo.20569239)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform: STM32H7](https://img.shields.io/badge/MCU-STM32H723-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32h723vg.html)

> Part of the custom optical digital MIDI piano project. Companion board:
> [**PIANO_G4_cbt6**](https://github.com/ScherbakovAl/PIANO_G4_cbt6) (sensor boards).
>
> Часть проекта самодельного цифрового MIDI-пианино на оптических датчиках.
> Парная плата: [**PIANO_G4_cbt6**](https://github.com/ScherbakovAl/PIANO_G4_cbt6) (платы-датчики).

---

## English

### Overview

**PIANO_H7** is the firmware for the **master controller board** of a self-built
digital MIDI piano that detects key strokes optically. Instead of rubber-dome
or simple contact switches, each key's hammer flies past optical sensors, and
the **hammer flight time** is measured to derive a realistic note velocity —
the same principle used in high-end digital pianos.

The instrument is split across two board types:

| Board | MCU | Role |
|-------|-----|------|
| **PIANO_H7** (this repo) | STM32H723VGTx | Master controller: UI, USB-MIDI, manages the sensor bus |
| [PIANO_G4_cbt6](https://github.com/ScherbakovAl/PIANO_G4_cbt6) | STM32G474 | Distributed sensor readers (comparators + timers) |

The boards were fabricated and assembled in late summer–autumn 2025 and are
used to play on a real instrument.

### What the H7 board does

- **Graphical UI** on an ST7796 TFT display with FT6336 capacitive touch,
  built with [LVGL](https://lvgl.io/): calibration screens, comparator
  settings, live charts of the optical sensor signals, and velocity-curve
  configuration.
- **USB-MIDI device** via [TinyUSB](https://github.com/hathach/tinyusb): the
  board enumerates as a class-compliant USB-MIDI device, so it plays into any
  DAW or host without drivers.
- **Sensor bus master:** communicates with up to 27 addressed G4 "chips"
  (addresses 1–13 for note-on sensors, 14–26 for note-off) over a multidrop
  UART (UART4), collecting hammer timings and pushing settings/calibration.
- **Velocity model:** converts hammer flight time (≈390–14000 µs) into a MIDI
  velocity using a physical model (key mass / travel distance) with separate
  ON/OFF curves.
- **Calibration storage:** comparator thresholds and calibration data are
  persisted in internal flash.
- **Bootloader / field update:** carries a custom bootloader. The H7 itself can
  be re-flashed over USB (USB DFU) using its own bootloader, and it can in turn
  flash the firmware of the G4 boards over the UART bus, including jumping
  between bootloader and main-application regions.

### GUI

The 320×480 touch screens, rendered on a PC by LVGL from the firmware's own UI
code (sample data; regenerate with `python3 eez_proj/рендер/рендер.py`):

<table>
<tr>
<td width="33%"><img src="eez_proj/%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D1%8B/1b_main_note.png" alt="Main screen"></td>
<td width="33%"><img src="eez_proj/%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D1%8B/2_curve.png" alt="Velocity curves"></td>
<td width="33%"><img src="eez_proj/%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D1%8B/3_calib.png" alt="Calibration"></td>
</tr>
<tr>
<td valign="top"><b>Main</b> — sensor-bus and USB status, the active velocity curve, the last note played (key, velocity, flight time), power off.</td>
<td valign="top"><b>Curves</b> — ON / OFF / LIFT tabs. Five ON curves; the user ones are tuned with sliders (time for vel 127 and vel 1, bend) and saved to flash. A crosshair shows where the last note fell on the curve. The selected curves and the "last note" row are remembered: saved to flash on return to the main screen.</td>
<td valign="top"><b>Calibration</b> — live signals of every optical sensor, for hammers and dampers. G/R comparator thresholds per key; step by key or by chip, or follow the key just played. A too-narrow G↔R gap is highlighted.</td>
</tr>
<tr>
<td><img src="eez_proj/%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D1%8B/4_service.png" alt="Service"></td>
<td><img src="eez_proj/%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D1%8B/5_flash.png" alt="G4 flashing"></td>
<td><img src="eez_proj/%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D1%8B/6_log_confirm.png" alt="Log and confirmation"></td>
</tr>
<tr>
<td valign="top"><b>Service</b> — polls all 26 G4 boards and shows which run the piano firmware or a bootloader; restore calibration from flash, USB DFU, reboot.</td>
<td valign="top"><b>G4 flashing</b> — step by step over the sensor bus: to the bootloader, write the firmware H7 → G4, start piano. Bootloader updates sit behind a "dangerous" fold.</td>
<td valign="top"><b>Log</b> — error counters (UART noise, lost MIDI, G4 ring buffer, LCD DMA, touch I2C) and the debug log. Irreversible actions ask for confirmation.</td>
</tr>
</table>

### Hardware

- MCU: **STM32H723VGTx**
- Display: ST7796 (TFT), touch: FT6336
- Sensor link: multidrop UART to the G4 boards
- PCB design, schematics and BOM are in [`pcb/`](pcb/) (EasyEDA Pro project,
  exported SCH/PCB PDFs and BOM).

3D render of the assembled boards, with the PCB layout and the controller-board
schematic:

![3D view of the assembled boards](pcb/PNG_H7/3D_H7.png)

- [EasyEDA Pro project (`.epro2`)](pcb/ProPrj_piano_base_v1.0_H7.epro2)
- [PCB layout ](pcb/PNG_H7/PCB_H7.png)
- [Schematic — controller / base board](pcb/PNG_H7/SCH_H7.png)

### Repository layout

```
Piano_H7/            Application firmware (piano_h7.cpp/.hpp, UI, drivers)
  ├─ st7796.*        TFT display driver
  ├─ ft6336.*        Capacitive touch driver
  ├─ lvgl/, ui/      LVGL library and generated UI
  └─ usb_descriptors USB-MIDI descriptors (TinyUSB)
Core/, Drivers/      STM32 HAL/LL, CMSIS, startup
H7_dfu_bootloader/   Custom bootloader for the H7
pcb/                 Schematics, PCB and BOM
cmake/, CMakeLists.txt, CMakePresets.json
```

### Building

The project uses CMake + the ARM GNU toolchain (configured via STM32CubeMX /
the included presets).

```bash
cmake --preset <preset>     # see CMakePresets.json
cmake --build --preset <preset>
```

Flash the resulting binary with your usual ST tool (ST-Link / DFU).

### Branches

| Branch | Status |
|--------|--------|
| `master` | Main branch. Matches the firmware currently running on the instrument. |
| `refact_to_vector_and_map` | Former development branch, merged into `master`. |
| `к-концерту-Кандинского` | Experiment: 5-byte H7↔G4 packet with explicit note-on/off events. Needs the branch of the same name in PIANO_G4_cbt6; incompatible with `master` of both repos. |
| `pwr` | Unfinished: standby / power control. |
| `for-concerts`, `для-сравнения-с-живым`, `для-тестирования-дома-редактора-миди-кривых` | Old experiments (autumn–winter 2025), kept for history. |

### License

Released under the [MIT License](LICENSE).

### Citation

If you use or reference this work, please cite it via its DOI (see the badge
above) or the [`CITATION.cff`](CITATION.cff) file.

---

## Русский

### Описание

**PIANO_H7** — это прошивка **главной платы-контроллера** самодельного
цифрового MIDI-пианино с оптическим считыванием нажатий. Вместо резиновых
куполов или простых контактов молоточек каждой клавиши пролетает мимо
оптических датчиков, и по **времени пролёта молоточка** вычисляется
реалистичная velocity ноты — тот же принцип, что в дорогих цифровых пиано.

Инструмент состоит из двух типов плат:

| Плата | МК | Роль |
|-------|-----|------|
| **PIANO_H7** (этот репозиторий) | STM32H723VGTx | Главный контроллер: интерфейс, USB-MIDI, управление шиной датчиков |
| [PIANO_G4_cbt6](https://github.com/ScherbakovAl/PIANO_G4_cbt6) | STM32G474 | Распределённые платы-датчики (компараторы + таймеры) |

Платы изготовлены и собраны в конце лета — осенью 2025 года и используются для
игры на реальном инструменте.

### Что делает плата H7

- **Графический интерфейс** на TFT-дисплее ST7796 с ёмкостным тачскрином
  FT6336, построенный на [LVGL](https://lvgl.io/): экраны калибровки,
  настройки компараторов, живые графики сигналов оптических датчиков,
  конфигурация кривых velocity.
- **USB-MIDI устройство** через [TinyUSB](https://github.com/hathach/tinyusb):
  плата определяется как class-compliant USB-MIDI устройство и играет в любой
  DAW/хост без драйверов.
- **Мастер шины датчиков:** обмен с до 27 адресуемыми «чипами» G4 (адреса 1–13
  — датчики note-on, 14–26 — note-off) по многоточечному UART (UART4): сбор
  таймингов молоточков, передача настроек и калибровки.
- **Модель velocity:** перевод времени пролёта молоточка (≈390–14000 мкс) в
  MIDI-velocity по физической модели (масса клавиши / расстояние) с раздельными
  кривыми ON/OFF.
- **Хранение калибровки:** пороги компараторов и данные калибровки сохраняются
  во внутренней flash-памяти.
- **Бутлоадер / обновление «в поле»:** собственный бутлоадер. Сам H7 может быть
  перепрошит по USB (USB DFU) через свой бутлоадер, а также умеет прошивать
  платы G4 по шине UART, включая переключение между областями бутлоадера и
  основного приложения.

### Интерфейс

Экраны 320×480 с тачем — отрисованы на ПК средствами LVGL из кода интерфейса
прошивки (данные-примеры; пересобрать — `python3 eez_proj/рендер/рендер.py`):

<table>
<tr>
<td width="33%"><img src="eez_proj/%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D1%8B/1b_main_note.png" alt="Главный экран"></td>
<td width="33%"><img src="eez_proj/%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D1%8B/2_curve.png" alt="Кривые velocity"></td>
<td width="33%"><img src="eez_proj/%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D1%8B/3_calib.png" alt="Калибровка"></td>
</tr>
<tr>
<td valign="top"><b>Главный</b> — состояние шины датчиков и USB, текущая кривая velocity, последняя сыгранная нота (клавиша, velocity, время пролёта), выключение.</td>
<td valign="top"><b>Кривые</b> — вкладки ON / OFF / LIFT. Пять кривых ON; пользовательские настраиваются ползунками (время для vel 127 и vel 1, изгиб) и сохраняются во flash. Перекрестье показывает, куда на кривой попала последняя нота. Выбранные кривые и строка «последняя нота» запоминаются: сохраняются во flash при возврате на главный экран.</td>
<td valign="top"><b>Калибровка</b> — живые сигналы всех оптических датчиков, для молоточков и демпферов. Пороги компараторов G/R для каждой клавиши; шаг по клавише или по чипу, или слежение за только что нажатой. Слишком узкий зазор G↔R подсвечивается.</td>
</tr>
<tr>
<td><img src="eez_proj/%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D1%8B/4_service.png" alt="Сервис"></td>
<td><img src="eez_proj/%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D1%8B/5_flash.png" alt="Прошивка G4"></td>
<td><img src="eez_proj/%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D1%8B/6_log_confirm.png" alt="Журнал и подтверждение"></td>
</tr>
<tr>
<td valign="top"><b>Сервис</b> — опрос всех 26 плат G4: какие в прошивке piano, какие в загрузчике; восстановление калибровки из памяти, USB DFU, перезагрузка.</td>
<td valign="top"><b>Прошивка G4</b> — по шагам через шину датчиков: в загрузчик, запись прошивки H7 → G4, запуск piano. Обновление самих загрузчиков — под «опасным» спойлером.</td>
<td valign="top"><b>Журнал</b> — счётчики ошибок (шум UART, потерянные MIDI, кольцевой буфер G4, LCD DMA, тач I2C) и отладочный лог. Необратимые действия требуют подтверждения.</td>
</tr>
</table>

### Аппаратная часть

- МК: **STM32H723VGTx**
- Дисплей: ST7796 (TFT), тач: FT6336
- Связь с датчиками: многоточечный UART к платам G4
- Файлы платы, схемы и BOM — в [`pcb/`](pcb/) (проект EasyEDA Pro,
  экспортированные PDF схемы/платы и BOM).

3D-вид собранных плат, трассировка платы и схема платы-контроллера:

![3D-вид собранных плат](pcb/PNG_H7/3D_H7.png)

- [Проект EasyEDA Pro (`.epro2`)](pcb/ProPrj_piano_base_v1.0_H7.epro2)
- [Трассировка платы](pcb/PNG_H7/PCB_H7.png)
- [Схема — плата-контроллер](pcb/PNG_H7/SCH_H7.png)

### Структура репозитория

```
Piano_H7/            Прошивка приложения (piano_h7.cpp/.hpp, UI, драйверы)
  ├─ st7796.*        Драйвер TFT-дисплея
  ├─ ft6336.*        Драйвер ёмкостного тача
  ├─ lvgl/, ui/      Библиотека LVGL и сгенерированный UI
  └─ usb_descriptors Дескрипторы USB-MIDI (TinyUSB)
Core/, Drivers/      STM32 HAL/LL, CMSIS, startup
H7_dfu_bootloader/   Собственный бутлоадер для H7
pcb/                 Схемы, плата и BOM
cmake/, CMakeLists.txt, CMakePresets.json
```

### Сборка

Проект использует CMake + ARM GNU toolchain (настройка через STM32CubeMX /
включённые пресеты).

```bash
cmake --preset <preset>     # см. CMakePresets.json
cmake --build --preset <preset>
```

Прошивка — обычным инструментом ST (ST-Link / DFU).

### Ветки

| Ветка | Состояние |
|-------|-----------|
| `master` | Основная ветка. Соответствует прошивке, которая сейчас работает в инструменте. |
| `refact_to_vector_and_map` | Бывшая ветка разработки, влита в `master`. |
| `к-концерту-Кандинского` | Эксперимент: 5-байтный пакет H7↔G4 с явными событиями note-on/off. Требует одноимённую ветку в PIANO_G4_cbt6; несовместима с `master` обоих репозиториев. |
| `pwr` | Не доделано: standby / управление питанием. |
| `for-concerts`, `для-сравнения-с-живым`, `для-тестирования-дома-редактора-миди-кривых` | Старые эксперименты (осень–зима 2025), сохранены для истории. |

### Лицензия

Распространяется под [лицензией MIT](LICENSE).

### Цитирование

При использовании или ссылке на эту работу, пожалуйста, цитируйте по DOI
(см. бейдж выше) или через файл [`CITATION.cff`](CITATION.cff).

(для плат изготовленных лето-осень 2025)