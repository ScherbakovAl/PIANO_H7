# План миграции PIANO_H7 с C++ на Rust

## Выбор архитектуры: RTIC + CMake-first

**Почему RTIC:**
- ✅ Без async - традиционный interrupt-driven подход
- ✅ Приоритетное планирование прерываний
- ✅ Встроенная поддержка STM32H7
- ✅ Zero-cost abstractions
- ✅ Безопасная модель памяти

---

## Структура проекта

```
PIANO_H7/
├── CMakeLists.txt              # CMake управляет сборкой (Rust lib + C)
├── Cargo.toml                  # Rust зависимости и embedded-hal
├── rust-toolchain              # nightly для RTIC
├── Core/                       # CubeMX HAL/LL (не трогаем)
├── Piano_H7/
│   ├── src/
│   │   ├── lib.rs              # Корневой модуль
│   │   ├── main.rs             # RTIC app
│   │   ├── hal/                # HAL обёртки (LL)
│   │   ├── chips/              # Логика чипов
│   │   ├── midi/               # Обработка MIDI
│   │   ├── ui/                 # UI слой
│   │   └── interrupts/         # Обработчики прерываний
│   ├── bindings/               # bindgen-generated
│   │   ├── lvgl.rs
│   │   └── tinyusb.rs
│   └── C/                      # C код для FFI
│       ├── lvgl_wrapper.c
│       └── tinyusb_wrapper.c
├── tinyusb/
├── cmake/
└── boards/
    └── piano_h7/               # BSP для платы
```

---

## Зависимости Rust (Cargo.toml)

```toml
[package]
name = "piano_h7"
version = "0.1.0"
edition = "2021"

[dependencies]
rtic = "2"                      # RTIC framework
cortex-m = "0.7"                # Cortex-M core
stm32h7xx-hal = { version = "0.20", features = ["stm32h723"] }
embedded-hal = "1.0"            # embedded HAL traits
nb = "1.0"                      # non-blocking
heapless = "0.8"                # no_std Vec/HashMap
static_assertions = "0.1"       # compile-time assertions

[dependencies.bindgen]
version = "0.70"
optional = true
```

---

## RTIC структура приложения

```rust
// Piano_H7/src/main.rs

use rtic::app;

#[app(device = stm32h7xx_hal::pac, dispatchers = [UART5, TIM4, TIM5])]
mod app {
    use super::*;

    #[shared]
    struct Resources {
        // Глобальное состояние
        chips: Vec<Chip>,
        buffers: Buffers,
        midi_sender: MidiSender,
        display_state: DisplayState,
    }

    #[local]
    struct Local {
        // Локальные ресурсы прерываний
        uart_rx: UART5,
        disp_flush: DMA2,
        touch: I2C5,
    }

    #[init]
    fn init(cx: init::Context) -> (Shared, Local, init::LateResources) {
        // 1. Инициализация периферии (из CubeMX HAL/LL)
        let rcc = cx.device.RCC.constrain();
        let clocks = rcc.cfgr.freeze();
        
        let uart5 = cx.device.UART5.constrain(115200.bps(), &clocks);
        let spi3 = cx.device.SPI3.constrain();
        let i2c5 = cx.device.I2C5.constrain();
        
        // 2. Инициализация LVGL (через FFI)
        lvgl_init();
        
        // 3. Инициализация tinyUSB
        tusb_init();
        
        // 4. Настройка прерываний
        let mut nvic = cx.core.NVIC;
        nvic.set_priority(priority!(DMA1_RX), 1);
        nvic.set_priority(priority!(TIM5), 2);
        
        let shared = Resources {
            chips: Vec::new(),
            buffers: Buffers::new(),
            midi_sender: MidiSender::new(),
            display_state: DisplayState::default(),
        };
        
        (shared, Local { uart_rx: uart5, disp_flush: dma2, touch: i2c5 }, {})
    }

    #[idle]
    fn idle(_cx: idle::Context) -> ! {
        loop {
            // LVGL timer handler
            lvgl_timer_handler();
            // USB task
            tud_task();
        }
    }

    #[task(binds = DMA1_RX, priority = 1, shared = [chips, buffers, midi_sender])]
    fn uart_rx_isr(cx: uart_rx_isr::Context) {
        // Обработка данных от G4 чипов
        let data = read_uart_buffer();
        process_chip_data(cx.shared, data);
    }

    #[task(binds = TIM5, priority = 2, shared = [chips, display_state])]
    fn calibration_tick(cx: calibration_tick::Context) {
        // Калибровочный тик (была в основном цикле)
        update_calibration(cx.shared);
    }
}
```

---

## Пошаговый план реализации

### Фаза 1: Подготовка окружения
- [ ] Установить Rust nightly toolchain
- [ ] Установить `cargo-generate` и `cargo-binutils`
- [ ] Настроить `.cargo/config.toml` для arm-none-eabi
- [ ] Создать `rust-toolchain` файл

### Фаза 2: Настройка CMake + Rust
- [ ] Модифицировать CMakeLists.txt для линковки Rust lib
- [ ] Добавить `embed_rust.c` entry point
- [ ] Настроить `Cargo.toml` с правильными features

### Фаза 3: FFI биндинги
- [ ] Сгенерировать `bindgen` для LVGL
- [ ] Сгенерировать `bindgen` для tinyusb
- [ ] Создать C-wrapper при необходимости

### Фаза 4: Перенос логики (по модулям)
- [ ] Модуль `chips` - структуры и логика работы с G4
- [ ] Модуль `midi` - расчёт velocity, формирование MIDI
- [ ] Модуль `ui` - UI события, графики
- [ ] Модуль `calibration` - калибровка сенсоров

### Фаза 5: Прерывания и DMA
- [ ] UART DMA прерывание (DMA1_RX)
- [ ] Display DMA (DMA2 Stream 1)
- [ ] Touch I2C DMA (DMA2 Stream 3)
- [ ] TIM5 для калибровки

### Фаза 6: Тестирование
- [ ] Компиляция и линковка
- [ ] Прошивка и отладка
- [ ] Проверка MIDI латентности
- [ ] Проверка UI responsiveness

---

## Ключевые особенности переноса

### 1. Структуры данных (C++ → Rust)

```cpp
// C++
struct comparator {
    uint8_t cursor;
    uint8_t address;
    uint8_t number_chip;
    uint8_t number_comparator;
    bool    is_active;
};

struct Chip {
    uint8_t number_chip;
    std::vector<comparator> comparators;
    typeAction typ;
    chip_states st;
};
```

```rust
// Rust
#[derive(Clone, Copy)]
struct Comparator {
    cursor: u8,
    address: u8,
    number_chip: u8,
    number_comparator: u8,
    is_active: bool,
}

struct Chip {
    number_chip: u8,
    comparators: heapless::Vec<Comparator, 7>,  // 7 comparators fixed
    typ: TypeAction,
    st: ChipState,
}
```

### 2. UART Communication (DMA)

```rust
// Обработка DMA буфера без копирования
#[repr(aligned(8))]
static RX_BUFFER: [u8; 4] = [0; 4];

fn start_uart_dma() {
    // Настройка DMA на RX_BUFFER
    let dma = unsafe { &*DMA1::ptr() };
    dma.st[2].mar.write(|w| maddr(&RX_BUFFER));
    dma.st[2].par.write(|w| paddr(&UART5.rdr));
    dma.st[2].ndtr.write(|w| w.ndt(4));
    dma.st[2].cr.modify(|_, w| w.en().enabled());
}
```

### 3. MIDI Message (Rust enum + FFI)

```rust
#[repr(u8)]
enum MidiCommand {
    NoteOn = 0x90,
    NoteOff = 0x80,
    ControlChange = 0xB0,
}

struct MidiMessage {
    command: MidiCommand,
    channel: u8,
    note: u8,
    velocity: u8,
}

impl MidiMessage {
    fn send(&self) {
        let buf = [
            self.command as u8 | self.channel,
            self.note,
            self.velocity,
        ];
        unsafe {
            tud_midi_stream_write(0, buf.as_ptr(), buf.len() as u16);
        }
    }
}
```

---

## Сравнение подходов

| Аспект | Ваш текущий C++ | Rust + RTIC |
|--------|-----------------|-------------|
| Безопасность памяти | manual | ownership |
| Real-time гарантии | прерывания | приоритеты RTIC |
| Сложность прерываний | race conditions | resource borrow |
| LVGL | C API | FFI bindings |
| USB MIDI | tinyusb | FFI + usbd-midi |
| Размер бинарника | ~200KB | ~150KB (с LTO) |

---

## Рекомендации по порядку переноса

1. **Сначала LVGL UI** - изолированный компонент, легко тестировать
2. **Потом MIDI расчёты** - чистый Rust без hardware dependencies
3. **Затем UART/DMA** - самый сложный для переноса
4. **Последним - main loop** - всё собирается вместе

---

## Файлы для создания/модификации

| Файл | Действие |
|------|----------|
| `Cargo.toml` | Создать |
| `rust-toolchain` | Создать |
| `.cargo/config.toml` | Создать |
| `Piano_H7/src/lib.rs` | Создать |
| `Piano_H7/src/main.rs` | Создать |
| `CMakeLists.txt` | Модифицировать |
| `Core/Src/main.c` | Модифицировать (call Rust) |
| `piano_h7/bindings/lvgl.rs` | Генерировать |
| `piano_h7/bindings/tinyusb.rs` | Генерировать |
