# Рекомендации по архитектуре данных калибровки для Piano H7

## Анализ текущей проблемы

### Текущие проблемы в коде:

1. **Разрозненные массивы данных:**
   - `compsCHART_0[196]` - зелёные точки (green)
   - `compsCHART_1[196]` - красные точки (red)
   - `compsCHART_CALIB[196]` - данные калибровки
   - Нет явной связи между индексом массива, номером чипа, номером компаратора и MIDI-нотой

2. **Небезопасные вычисления индексов:**
   ```cpp
   // Строки 277, 293, 467, 471, 480, 486, 1551, 1579, 1592
   for (uint8_t adress = start_adress_chip_on; adress <= end_adress_chip_on; ++adress) {
       // TODO numbers_chips - ненадёжно!
   ```

3. **Магические числа:**
   - `cursor - 7`, `cursor + 98` - непонятная логика смещений
   - Жёстко зашитые границы: `< 98`, `>= 98`

4. **Формат UART сообщений:**
   - `tx_settings[5]` и `rx_settings[5]` - фиксированный размер
   - Формат: `[command][compN][dot][value_hi][value_lo]`

---

## Предлагаемая архитектура

### 1. Базовые типы данных

```cpp
#pragma once
#include <cstdint>
#include <array>
#include <optional>

namespace Piano {

// ============================================================================
// КОНСТАНТЫ СИСТЕМЫ
// ============================================================================

constexpr uint8_t COMPARATORS_PER_CHIP = 7;
constexpr uint8_t MAX_CHIPS = 27;
constexpr uint8_t TOTAL_KEYS = 196;

// Диапазоны адресов чипов
constexpr uint8_t CHIP_ON_START = 5;   // Молоточки (hammers)
constexpr uint8_t CHIP_ON_END = 7;     // 3 чипа * 7 = 21 нота (но используется до 98)
constexpr uint8_t CHIP_OFF_START = 14; // Демпферы (dampers)
constexpr uint8_t CHIP_OFF_END = 10;   // Обратный порядок!

constexpr uint8_t HAMMERS_COUNT = 98;  // Молоточки: индексы 0-97
constexpr uint8_t DAMPERS_COUNT = 98;  // Демпферы: индексы 98-195

// Точки срабатывания
constexpr uint8_t HAMMER_THRESHOLDS = 2;  // Green, Red
constexpr uint8_t DAMPER_THRESHOLDS = 3;  // Green, Red, Hysteresis

// ============================================================================
// БАЗОВЫЕ ТИПЫ
// ============================================================================

// Тип для хранения значения компаратора (12-bit ADC)
struct ComparatorValue {
    uint16_t value;  // 0-4095
    
    constexpr ComparatorValue() : value(0) {}
    constexpr explicit ComparatorValue(uint16_t v) : value(v > 4095 ? 4095 : v) {}
    
    constexpr bool isValid() const { return value <= 4095; }
    constexpr operator uint16_t() const { return value; }
};

// Координаты компаратора в системе
struct ComparatorAddress {
    uint8_t chip_address;      // Физический адрес чипа (5-7, 14-10)
    uint8_t comparator_index;  // Номер компаратора на чипе (0-6)
    
    constexpr bool isValid() const {
        return comparator_index < COMPARATORS_PER_CHIP &&
               ((chip_address >= CHIP_ON_START && chip_address <= CHIP_ON_END) ||
                (chip_address >= CHIP_OFF_END && chip_address <= CHIP_OFF_START));
    }
};

// Точка калибровки для одного компаратора
struct ComparatorThresholds {
    ComparatorValue green;       // Первая точка срабатывания
    ComparatorValue red;         // Вторая точка срабатывания
    ComparatorValue hysteresis;  // Третья точка (только для демпферов)
    
    constexpr ComparatorThresholds() 
        : green(0), red(0), hysteresis(0) {}
    
    constexpr ComparatorThresholds(uint16_t g, uint16_t r, uint16_t h = 0)
        : green(g), red(r), hysteresis(h) {}
};

// ============================================================================
// ИНДЕКСАЦИЯ И КОНВЕРТАЦИЯ
// ============================================================================

// Глобальный индекс клавиши (0-195)
struct KeyIndex {
    uint8_t index;  // 0-97: молоточки, 98-195: демпферы
    
    constexpr KeyIndex() : index(0) {}
    constexpr explicit KeyIndex(uint8_t i) : index(i < TOTAL_KEYS ? i : 0) {}
    
    constexpr bool isHammer() const { return index < HAMMERS_COUNT; }
    constexpr bool isDamper() const { return index >= HAMMERS_COUNT; }
    constexpr bool isValid() const { return index < TOTAL_KEYS; }
    
    // Конвертация в адрес компаратора
    constexpr ComparatorAddress toComparatorAddress() const {
        if (isHammer()) {
            // Молоточки: индексы 0-97 -> чипы 5-7
            uint8_t chip_offset = index / COMPARATORS_PER_CHIP;
            return {
                static_cast<uint8_t>(CHIP_ON_START + chip_offset),
                static_cast<uint8_t>(index % COMPARATORS_PER_CHIP)
            };
        } else {
            // Демпферы: индексы 98-195 -> чипы 14-10 (обратный порядок!)
            uint8_t damper_index = index - HAMMERS_COUNT;
            uint8_t chip_offset = damper_index / COMPARATORS_PER_CHIP;
            return {
                static_cast<uint8_t>(CHIP_OFF_START - chip_offset),
                static_cast<uint8_t>(damper_index % COMPARATORS_PER_CHIP)
            };
        }
    }
    
    // Конвертация в MIDI ноту
    constexpr uint8_t toMidiNote(const int8_t* noteAdder) const {
        if (!isValid() || !noteAdder) return 0;
        return static_cast<uint8_t>(index + noteAdder[index]);
    }
};

// Обратная конвертация: адрес компаратора -> индекс клавиши
constexpr std::optional<KeyIndex> fromComparatorAddress(ComparatorAddress addr) {
    if (!addr.isValid()) return std::nullopt;
    
    // Молоточки
    if (addr.chip_address >= CHIP_ON_START && addr.chip_address <= CHIP_ON_END) {
        uint8_t chip_offset = addr.chip_address - CHIP_ON_START;
        uint8_t index = chip_offset * COMPARATORS_PER_CHIP + addr.comparator_index;
        if (index < HAMMERS_COUNT) {
            return KeyIndex(index);
        }
    }
    
    // Демпферы (обратный порядок адресов!)
    if (addr.chip_address >= CHIP_OFF_END && addr.chip_address <= CHIP_OFF_START) {
        uint8_t chip_offset = CHIP_OFF_START - addr.chip_address;
        uint8_t index = HAMMERS_COUNT + chip_offset * COMPARATORS_PER_CHIP + addr.comparator_index;
        if (index < TOTAL_KEYS) {
            return KeyIndex(index);
        }
    }
    
    return std::nullopt;
}

// ============================================================================
// ОСНОВНОЙ КЛАСС ДАННЫХ КАЛИБРОВКИ
// ============================================================================

class CalibrationData {
private:
    // Основное хранилище данных - выровнено для flash
    alignas(32) std::array<ComparatorThresholds, TOTAL_KEYS> thresholds_;
    
    // Текущие значения с компараторов (для режима калибровки)
    std::array<uint16_t, TOTAL_KEYS> current_values_;
    
    // Флаги изменений для оптимизации записи
    std::array<bool, TOTAL_KEYS> modified_;
    
public:
    CalibrationData() {
        initDefaults();
    }
    
    // Инициализация значениями по умолчанию
    void initDefaults() {
        for (uint8_t i = 0; i < TOTAL_KEYS; ++i) {
            KeyIndex key(i);
            if (key.isHammer()) {
                thresholds_[i] = ComparatorThresholds(2600, 1000, 0);
            } else {
                thresholds_[i] = ComparatorThresholds(2100, 2400, 1890); // hysteresis = green - 10%
            }
            current_values_[i] = 800;
            modified_[i] = false;
        }
    }
    
    // ========================================================================
    // ДОСТУП К ДАННЫМ ПО ИНДЕКСУ КЛАВИШИ
    // ========================================================================
    
    ComparatorThresholds& operator[](KeyIndex key) {
        if (!key.isValid()) {
            static ComparatorThresholds dummy;
            return dummy;
        }
        modified_[key.index] = true;
        return thresholds_[key.index];
    }
    
    const ComparatorThresholds& operator[](KeyIndex key) const {
        static const ComparatorThresholds dummy;
        return key.isValid() ? thresholds_[key.index] : dummy;
    }
    
    // ========================================================================
    // ДОСТУП К ДАННЫМ ПО АДРЕСУ КОМПАРАТОРА
    // ========================================================================
    
    std::optional<ComparatorThresholds*> get(ComparatorAddress addr) {
        auto key = fromComparatorAddress(addr);
        if (!key) return std::nullopt;
        modified_[key->index] = true;
        return &thresholds_[key->index];
    }
    
    std::optional<const ComparatorThresholds*> get(ComparatorAddress addr) const {
        auto key = fromComparatorAddress(addr);
        if (!key) return std::nullopt;
        return &thresholds_[key->index];
    }
    
    // ========================================================================
    // РАБОТА С ТЕКУЩИМИ ЗНАЧЕНИЯМИ (РЕЖИМ КАЛИБРОВКИ)
    // ========================================================================
    
    void setCurrentValue(KeyIndex key, uint16_t value) {
        if (key.isValid()) {
            current_values_[key.index] = value;
        }
    }
    
    uint16_t getCurrentValue(KeyIndex key) const {
        return key.isValid() ? current_values_[key.index] : 0;
    }
    
    // ========================================================================
    // РАБОТА С LVGL CHART
    // ========================================================================
    
    // Получить указатель на массив для LVGL (зелёные точки)
    int32_t* getGreenArrayForChart(bool isHammer) {
        static std::array<int32_t, HAMMERS_COUNT> hammer_green;
        static std::array<int32_t, DAMPERS_COUNT> damper_green;
        
        if (isHammer) {
            for (uint8_t i = 0; i < HAMMERS_COUNT; ++i) {
                hammer_green[i] = thresholds_[i].green.value;
            }
            return hammer_green.data();
        } else {
            for (uint8_t i = 0; i < DAMPERS_COUNT; ++i) {
                damper_green[i] = thresholds_[HAMMERS_COUNT + i].green.value;
            }
            return damper_green.data();
        }
    }
    
    // Получить указатель на массив для LVGL (красные точки)
    int32_t* getRedArrayForChart(bool isHammer) {
        static std::array<int32_t, HAMMERS_COUNT> hammer_red;
        static std::array<int32_t, DAMPERS_COUNT> damper_red;
        
        if (isHammer) {
            for (uint8_t i = 0; i < HAMMERS_COUNT; ++i) {
                hammer_red[i] = thresholds_[i].red.value;
            }
            return hammer_red.data();
        } else {
            for (uint8_t i = 0; i < DAMPERS_COUNT; ++i) {
                damper_red[i] = thresholds_[HAMMERS_COUNT + i].red.value;
            }
            return damper_red.data();
        }
    }
    
    // Получить указатель на массив текущих значений для LVGL
    int32_t* getCurrentArrayForChart(bool isHammer) {
        static std::array<int32_t, HAMMERS_COUNT> hammer_current;
        static std::array<int32_t, DAMPERS_COUNT> damper_current;
        
        if (isHammer) {
            for (uint8_t i = 0; i < HAMMERS_COUNT; ++i) {
                hammer_current[i] = current_values_[i];
            }
            return hammer_current.data();
        } else {
            for (uint8_t i = 0; i < DAMPERS_COUNT; ++i) {
                damper_current[i] = current_values_[HAMMERS_COUNT + i];
            }
            return damper_current.data();
        }
    }
    
    // ========================================================================
    // UART ПРОТОКОЛ
    // ========================================================================
    
    // Формат сообщения: [command][compN][dot][value_hi][value_lo]
    struct UartMessage {
        uint8_t data[5];
        
        void setSetCommand(ComparatorAddress addr, uint8_t threshold_index, uint16_t value) {
            data[0] = static_cast<uint8_t>(command::set_comp_value);
            data[1] = addr.comparator_index;
            data[2] = threshold_index;  // 0=green, 1=red, 2=hysteresis
            data[3] = (value >> 8) & 0xFF;
            data[4] = value & 0xFF;
        }
        
        void setReadCommand(ComparatorAddress addr, uint8_t threshold_index) {
            data[0] = static_cast<uint8_t>(command::read_comp_value);
            data[1] = addr.comparator_index;
            data[2] = threshold_index;
            data[3] = 0;
            data[4] = 0;
        }
        
        uint16_t extractValue() const {
            return (static_cast<uint16_t>(data[3]) << 8) | data[4];
        }
    };
    
    // Отправить все данные на чип
    template<typename SendFunc>
    void sendToChip(uint8_t chip_address, SendFunc send_uart) {
        for (uint8_t comp = 0; comp < COMPARATORS_PER_CHIP; ++comp) {
            ComparatorAddress addr{chip_address, comp};
            auto key = fromComparatorAddress(addr);
            if (!key) continue;
            
            const auto& thresh = thresholds_[key->index];
            
            // Отправка green
            UartMessage msg;
            msg.setSetCommand(addr, 0, thresh.green.value);
            send_uart(chip_address, msg.data, sizeof(msg.data));
            
            // Отправка red
            msg.setSetCommand(addr, 1, thresh.red.value);
            send_uart(chip_address, msg.data, sizeof(msg.data));
            
            // Отправка hysteresis (только для демпферов)
            if (key->isDamper()) {
                msg.setSetCommand(addr, 2, thresh.hysteresis.value);
                send_uart(chip_address, msg.data, sizeof(msg.data));
            }
        }
    }
    
    // Прочитать все данные с чипа
    template<typename SendFunc, typename ReceiveFunc>
    void readFromChip(uint8_t chip_address, SendFunc send_uart, ReceiveFunc receive_uart) {
        for (uint8_t comp = 0; comp < COMPARATORS_PER_CHIP; ++comp) {
            ComparatorAddress addr{chip_address, comp};
            auto key = fromComparatorAddress(addr);
            if (!key) continue;
            
            UartMessage msg;
            
            // Чтение green
            msg.setReadCommand(addr, 0);
            send_uart(chip_address, msg.data, sizeof(msg.data));
            receive_uart(msg.data, sizeof(msg.data));
            thresholds_[key->index].green = ComparatorValue(msg.extractValue());
            
            // Чтение red
            msg.setReadCommand(addr, 1);
            send_uart(chip_address, msg.data, sizeof(msg.data));
            receive_uart(msg.data, sizeof(msg.data));
            thresholds_[key->index].red = ComparatorValue(msg.extractValue());
            
            // Чтение hysteresis (только для демпферов)
            if (key->isDamper()) {
                msg.setReadCommand(addr, 2);
                send_uart(chip_address, msg.data, sizeof(msg.data));
                receive_uart(msg.data, sizeof(msg.data));
                thresholds_[key->index].hysteresis = ComparatorValue(msg.extractValue());
            }
        }
    }
    
    // ========================================================================
    // СОХРАНЕНИЕ/ЗАГРУЗКА ИЗ FLASH
    // ========================================================================
    
    // Получить указатель на данные для записи в flash
    const uint32_t* getFlashData() const {
        return reinterpret_cast<const uint32_t*>(thresholds_.data());
    }
    
    // Размер данных в байтах
    constexpr size_t getFlashDataSize() const {
        return sizeof(thresholds_);
    }
    
    // Загрузить данные из flash
    void loadFromFlash(const uint32_t* flash_address) {
        const auto* src = reinterpret_cast<const ComparatorThresholds*>(flash_address);
        for (size_t i = 0; i < TOTAL_KEYS; ++i) {
            thresholds_[i] = src[i];
            modified_[i] = false;
        }
    }
    
    // Проверка контрольной суммы (простая)
    uint32_t calculateChecksum() const {
        uint32_t sum = 0;
        const uint32_t* data = getFlashData();
        size_t count = getFlashDataSize() / sizeof(uint32_t);
        for (size_t i = 0; i < count; ++i) {
            sum ^= data[i];
        }
        return sum;
    }
    
    // ========================================================================
    // УТИЛИТЫ
    // ========================================================================
    
    void clearModifiedFlags() {
        modified_.fill(false);
    }
    
    bool isModified(KeyIndex key) const {
        return key.isValid() ? modified_[key.index] : false;
    }
    
    // Получить диапазон min/max для отображения
    struct MinMax {
        uint16_t min;
        uint16_t max;
    };
    
    MinMax getGreenRange(bool isHammer) const {
        MinMax result{4095, 0};
        uint8_t start = isHammer ? 0 : HAMMERS_COUNT;
        uint8_t end = isHammer ? HAMMERS_COUNT : TOTAL_KEYS;
        
        for (uint8_t i = start; i < end; ++i) {
            uint16_t val = thresholds_[i].green.value;
            if (val < result.min) result.min = val;
            if (val > result.max) result.max = val;
        }
        return result;
    }
    
    MinMax getRedRange(bool isHammer) const {
        MinMax result{4095, 0};
        uint8_t start = isHammer ? 0 : HAMMERS_COUNT;
        uint8_t end = isHammer ? HAMMERS_COUNT : TOTAL_KEYS;
        
        for (uint8_t i = start; i < end; ++i) {
            uint16_t val = thresholds_[i].red.value;
            if (val < result.min) result.min = val;
            if (val > result.max) result.max = val;
        }
        return result;
    }
};

} // namespace Piano
```

---

## 2. Примеры использования

### Пример 1: Отправка данных на все чипы (замена all_H7_to_g4)

```cpp
void all_H7_to_g4_new(Piano::CalibrationData& calib) {
    // Молоточки
    for (uint8_t chip = CHIP_ON_START; chip <= CHIP_ON_END; ++chip) {
        calib.sendToChip(chip, [](uint8_t addr, const uint8_t* data, size_t len) {
            UART4_SendAddress(addr);
            pause(3);
            // Отправка data через UART
            for (size_t i = 0; i < len; ++i) {
                LL_USART_TransmitData9(UART5, data[i]);
                while (!LL_USART_IsActiveFlag_TXE(UART5)) {}
            }
            while (!LL_USART_IsActiveFlag_TC(UART5)) {}
            pause(1);
        });
    }
    
    // Демпферы
    for (uint8_t chip = CHIP_OFF_START; chip >= CHIP_OFF_END; --chip) {
        calib.sendToChip(chip, /* ... */);
    }
}
```

### Пример 2: Обновление графика LVGL

```cpp
void configCharts_new(Piano::CalibrationData& calib) {
    lv_obj_t* chart_on = objects.chart_on;
    lv_chart_set_point_count(chart_on, 89);
    
    // Используем безопасные указатели на массивы
    ser_on_green = lv_chart_add_series(chart_on, lv_color_hex(0x0aaa37), LV_CHART_AXIS_PRIMARY_X);
    ser_on_red = lv_chart_add_series(chart_on, lv_color_hex(0xdb591e), LV_CHART_AXIS_PRIMARY_X);
    ser_on_blue = lv_chart_add_series(chart_on, lv_color_hex(0x314ded), LV_CHART_AXIS_PRIMARY_X);
    
    lv_chart_set_series_ext_y_array(chart_on, ser_on_green, calib.getGreenArrayForChart(true));
    lv_chart_set_series_ext_y_array(chart_on, ser_on_red, calib.getRedArrayForChart(true));
    lv_chart_set_series_ext_y_array(chart_on, ser_on_blue, calib.getCurrentArrayForChart(true));
    
    // Автоматический расчёт диапазонов
    auto green_range = calib.getGreenRange(true);
    auto red_range = calib.getRedRange(true);
    
    lv_chart_set_axis_range(chart_on, LV_CHART_AXIS_PRIMARY_Y, green_range.min, green_range.max);
    lv_chart_set_axis_range(chart_on, LV_CHART_AXIS_SECONDARY_Y, red_range.min, red_range.max);
}
```

### Пример 3: Обработка прерывания DMA (режим игры)

```cpp
void DMA1_RX_new(Piano::CalibrationData& calib) {
    LL_DMA_ClearFlag_TC2(DMA1);
    LL_TIM_DisableCounter(TIM1);
    
    SCB_CleanInvalidateDCache();
    
    // rx_data[0] - это индекс в старой системе (0-195)
    Piano::KeyIndex key(rx_data[0]);
    
    if (!key.isValid()) {
        USART_Noise_Error_detected();
        return;
    }
    
    // Получаем MIDI ноту безопасно
    uint8_t midi_note = key.toMidiNote(noteAdder);
    
    // Вычисление velocity...
    uint32_t tOut = rx_data[1] << 16 | rx_data[2] << 8 | rx_data[3];
    // ... расчёт midi_hi_F, midi_lo_F ...
    
    uint8_t note_buf[] = {
        0xB0,
        0x58,
        (uint8_t)midi_lo_F,
        key.isHammer() ? 0x90 : 0x80,  // Безопасное определение типа
        midi_note,
        (uint8_t)midi_hi_F
    };
    
    tud_midi_stream_write(0, note_buf, 6);
    
    LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_2);
    LL_TIM_EnableCounter(TIM1);
}
```

### Пример 4: Калибровка с дисплея

```cpp
void action_calib_sensor_1_on_new(lv_event_t* e, Piano::CalibrationData& calib) {
    Piano::KeyIndex key(cursor);  // cursor - глобальная переменная
    
    if (!key.isValid() || !key.isHammer()) return;
    
    // Получаем адрес компаратора
    auto addr = key.toComparatorAddress();
    
    // Получаем текущее значение с компаратора
    uint16_t current = calib.getCurrentValue(key);
    
    // Устанавливаем как зелёную точку
    calib[key].green = Piano::ComparatorValue(current);
    
    // Отправляем на чип
    Piano::CalibrationData::UartMessage msg;
    msg.setSetCommand(addr, 0, current);
    
    UART4_SendAddress(addr.chip_address);
    pause(3);
    // Отправка msg.data через UART...
    
    // Обновляем дисплей
    sensor_on_1_data_string = std::to_string(current);
    lv_chart_refresh(cur_shart);
}
```

### Пример 5: Сохранение в Flash

```cpp
void SaveToMemory_new(const Piano::CalibrationData& calib) {
    SCB_DisableICache();
    SCB_DisableDCache();
    HAL_FLASH_Unlock();
    
    FLASH_Erase_Sector(FLASH_SECTOR_7, FLASH_BANK_1, FLASH_VOLTAGE_RANGE_2);
    
    const uint32_t* data = calib.getFlashData();
    size_t size = calib.getFlashDataSize();
    uint32_t addr = FLASH_ADDRESS;
    
    // Запись данных (по 32 байта - FLASHWORD)
    for (size_t i = 0; i < size; i += 32) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, addr, (uint32_t)&data[i/4]) != HAL_OK) {
            HAL_FLASH_Lock();
            return;
        }
        addr += 32;
    }
    
    // Запись контрольной суммы
    uint32_t checksum = calib.calculateChecksum();
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, addr, (uint32_t)&checksum);
    
    HAL_FLASH_Lock();
    SCB_EnableICache();
    SCB_EnableDCache();
}

void ReadOnMemory_new(Piano::CalibrationData& calib) {
    const uint32_t* flash_data = reinterpret_cast<const uint32_t*>(FLASH_ADDRESS);
    calib.loadFromFlash(flash_data);
    
    // Проверка контрольной суммы
    uint32_t stored_checksum = *(flash_data + calib.getFlashDataSize() / sizeof(uint32_t));
    if (stored_checksum != calib.calculateChecksum()) {
        debugg_fn("Flash checksum error! Using defaults.");
        calib.initDefaults();
    }
}
```

---

## 3. Дополнительные рекомендации

### 3.1 Управление активными чипами

Вместо жёстко заданных диапазонов используйте динамический список:

```cpp
class ChipManager {
private:
    std::array<bool, MAX_CHIPS> active_chips_;
    
public:
    ChipManager() {
        active_chips_.fill(false);
    }
    
    void scanChips() {
        // Сканирование через G4_echo()
        for (uint8_t addr = 0; addr < MAX_CHIPS; ++addr) {
            // Попытка связи с чипом
            if (tryEcho(addr)) {
                active_chips_[addr] = true;
            }
        }
    }
    
    template<typename Func>
    void forEachActiveChip(Func func) {
        for (uint8_t addr = 0; addr < MAX_CHIPS; ++addr) {
            if (active_chips_[addr]) {
                func(addr);
            }
        }
    }
    
    bool isActive(uint8_t chip_address) const {
        return chip_address < MAX_CHIPS && active_chips_[chip_address];
    }
};
```

Использование:

```cpp
ChipManager chip_mgr;

void sync_new() {
    chip_mgr.scanChips();
    
    chip_mgr.forEachActiveChip([](uint8_t addr) {
        sync_sender(addr);
    });
}

void all_H7_to_g4_new(CalibrationData& calib) {
    chip_mgr.forEachActiveChip([&calib](uint8_t addr) {
        calib.sendToChip(addr, /* ... */);
    });
}
```

### 3.2 Безопасность типов

Используйте `enum class` вместо `enum`:

```cpp
enum class Command : uint8_t {
    SyncTimer = 1,
    Calibrate,
    ReadCompValue,
    SetCompValue,
    ReadCompSetting,
    AllCalib,
    ResetToBootloader,
    Echo = 11
};

enum class ThresholdType : uint8_t {
    Green = 0,
    Red = 1,
    Hysteresis = 2
};
```

### 3.3 RAII для управления ресурсами

```cpp
class UartTransaction {
public:
    UartTransaction() {
        LL_TIM_DisableCounter(TIM1);
        LL_USART_DisableDMAReq_RX(UART5);
    }
    
    ~UartTransaction() {
        LL_USART_EnableDMAReq_RX(UART5);
        LL_TIM_EnableCounter(TIM1);
    }
    
    UartTransaction(const UartTransaction&) = delete;
    UartTransaction& operator=(const UartTransaction&) = delete;
};

// Использование:
void all_H7_to_g4_safe(CalibrationData& calib) {
    UartTransaction transaction;  // Автоматически отключит/включит UART и таймер
    
    chip_mgr.forEachActiveChip([&calib](uint8_t addr) {
        calib.sendToChip(addr, /* ... */);
    });
    
    // Деструктор автоматически восстановит состояние
}
```

### 3.4 Оптимизация режима калибровки

```cpp
class CalibrationMode {
private:
    CalibrationData& calib_;
    bool is_active_;
    uint32_t last_update_time_;
    
public:
    CalibrationMode(CalibrationData& calib) 
        : calib_(calib), is_active_(false), last_update_time_(0) {}
    
    void start(bool is_hammer) {
        is_active_ = true;
        // Отправка команды start_calibration на все чипы
    }
    
    void stop() {
        is_active_ = false;
        // Отправка команды stop_calibration
    }
    
    void update() {
        if (!is_active_) return;
        
        // Ограничение частоты обновления (TIM5->CNT > 3000)
        if (TIM5->CNT < 3000) return;
        TIM5->CNT = 0;
        
        // Опрос всех активных чипов
        chip_mgr.forEachActiveChip([this](uint8_t addr) {
            readCalibrationValues(addr);
        });
        
        // Обновление дисплея
        lv_chart_refresh(cur_shart);
    }
    
private:
    void readCalibrationValues(uint8_t chip_addr) {
        for (uint8_t comp = 0; comp < COMPARATORS_PER_CHIP; ++comp) {
            ComparatorAddress addr{chip_addr, comp};
            auto key = fromComparatorAddress(addr);
            if (!key) continue;
            
            // Чтение текущего значения
            uint16_t value = readCurrentValueFromChip(addr);
            calib_.setCurrentValue(*key, value);
        }
    }
};
```

---

## 4. Миграция существующего кода

### Шаг 1: Создать глобальный экземпляр

```cpp
// В piano_h7.cpp
Piano::CalibrationData g_calibration;
Piano::ChipManager g_chip_manager;
```

### Шаг 2: Заменить массивы

```cpp
// Старый код:
// int32_t compsCHART_0[sizeCHART_BUFFER] = {};
// int32_t compsCHART_1[sizeCHART_BUFFER] = {};

// Новый код: данные хранятся внутри g_calibration
```

### Шаг 3: Обновить функции по одной

Начните с простых функций:
1. `initBuffers()` → `g_calibration.initDefaults()`
2. `SaveToMemory()` → `SaveToMemory_new(g_calibration)`
3. `ReadOnMemory()` → `ReadOnMemory_new(g_calibration)`

Затем более сложные:
4. `all_H7_to_g4()` → использовать `sendToChip()`
5. `all_g4_to_H7()` → использовать `readFromChip()`
6. `configCharts()` → использовать `getGreenArrayForChart()` и т.д.

### Шаг 4: Обновить обработчики событий

Все `action_*` функции должны работать с `g_calibration` через `KeyIndex`.

---

## 5. Преимущества новой архитектуры

### ✅ Безопасность типов
- Невозможно случайно использовать неверный индекс
- Компилятор проверяет корректность на этапе компиляции
- `constexpr` функции позволяют вычислять индексы в compile-time

### ✅ Читаемость
- Явная семантика: `key.isHammer()` вместо `rxB < 98`
- Самодокументируемый код
- Нет магических чисел

### ✅ Поддерживаемость
- Изменение количества чипов требует изменения только констант
- Легко добавить новые типы данных калибровки
- Централизованная логика конвертации

### ✅ Производительность
- `constexpr` функции выполняются в compile-time
- Нет накладных расходов в runtime
- Оптимизация компилятором благодаря `inline`

### ✅ Совместимость
- Формат UART сообщений сохранён (5 байт)
- Формат Flash данных можно сделать совместимым
- Постепенная миграция возможна

---

## 6. Потенциальные проблемы и решения

### Проблема 1: Размер кода увеличится

**Решение:** Используйте `-Os` (оптимизация по размеру) и LTO (Link Time Optimization):
```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -flto")
```

### Проблема 2: Статические массивы в `getGreenArrayForChart()`

**Решение:** Если памяти мало, используйте указатели напрямую:
```cpp
// Вместо копирования создайте view
struct ChartView {
    int32_t* data;
    size_t size;
};
```

### Проблема 3: Обратная совместимость с Flash

**Решение:** Добавьте версионирование:
```cpp
struct FlashHeader {
    uint32_t magic;      // 0xCAFEBABE
    uint32_t version;    // 1
    uint32_t checksum;
    uint32_t reserved;
};
```

---

## 7. Тестирование

### Unit-тесты (на PC)

```cpp
#include <cassert>

void test_key_index_conversion() {
    // Тест молоточков
    Piano::KeyIndex key(0);
    auto addr = key.toComparatorAddress();
    assert(addr.chip_address == 5);
    assert(addr.comparator_index == 0);
    
    // Тест демпферов
    Piano::KeyIndex key2(98);
    auto addr2 = key2.toComparatorAddress();
    assert(addr2.chip_address == 14);
    assert(addr2.comparator_index == 0);
    
    // Обратная конвертация
    auto key_back = Piano::fromComparatorAddress(addr);
    assert(key_back.has_value());
    assert(key_back->index == 0);
}

void test_calibration_data() {
    Piano::CalibrationData calib;
    calib.initDefaults();
    
    Piano::KeyIndex key(0);
    assert(calib[key].green.value == 2600);
    assert(calib[key].red.value == 1000);
    
    Piano::KeyIndex key2(98);
    assert(calib[key2].green.value == 2100);
    assert(calib[key2].red.value == 2400);
}
```

---

## Заключение

Предложенная архитектура решает все выявленные проблемы:

1. ✅ **Безопасная индексация** через `KeyIndex` и `ComparatorAddress`
2. ✅ **Удобная конвертация** между индексами, адресами, MIDI-нотами
3. ✅ **Инкапсуляция данных** в `CalibrationData`
4. ✅ **Сохранение формата UART** (5 байт)
5. ✅ **Простая интеграция с LVGL** через `getArrayForChart()`
6. ✅ **Поддержка Flash** с контрольной суммой
7. ✅ **Динамическое управление чипами** через `ChipManager`
8. ✅ **RAII для безопасности** ресурсов

Код становится более надёжным, читаемым и поддерживаемым без потери производительности.
