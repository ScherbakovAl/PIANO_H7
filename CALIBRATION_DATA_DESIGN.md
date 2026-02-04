# Рекомендации по организации типа данных для калибровки пианино

## Анализ текущего состояния

### Выявленные проблемы

1. **Отсутствие типобезопасности**: данные калибровки хранятся в плоских массивах `compsCHART_0`, `compsCHART_1` без какой-либо абстракции над ними.

2. **Громоздкие вычисления**: код содержит множество мест с `i / 7` и `i % 7` для доступа к данным конкретного чипа/компаратора.

3. **Магические константы**: `start_adress_chip_on = 5`, `end_adress_chip_on = 7`, `98 = 7*14` и т.д. разбросаны по коду.

4. **Использование `std::string`**: в embedded-системе это создаёт проблемы с памятью и производительностью.

5. **Дублирование данных**: `compsCHART_0` и `compsCHART_1` содержат пересекающиеся данные для `off` режима.

6. **Нет единой точки правки**: изменение данных происходит через разные места кода с разной логикой.

---

## Предлагаемая структура данных

### Основные константы

```cpp
#pragma once

#include <cstdint>
#include <array>
#include <bit>
#include <span>
#include <optional>

namespace piano {

// Конфигурация системы
inline constexpr std::size_t MAX_CHIPS = 27;          // Максимальное количество чипов G4
inline constexpr std::size_t COMPARATORS_PER_CHIP = 7; // Компараторов на каждом чипе
inline constexpr std::size_t TRIGGER_POINTS_HAMMER = 2;// Точек срабатывания для молоточка (on)
inline constexpr std::size_t TRIGGER_POINTS_DAMPER = 3;// Точки срабатывания для демпфера (off)
inline constexpr std::size_t SENSORS_PER_COMPARATOR = 2;// Датчиков на компаратор (green, red)

// Общее количество нот (клавиш)
inline constexpr std::size_t TOTAL_NOTES = 196;        // 98 on + 98 off

// Размеры буферов для дисплея
inline constexpr std::size_t CHART_BUFFER_SIZE = 196;
inline constexpr std::size_t ON_NOTES_COUNT = 98;
inline constexpr std::size_t OFF_NOTES_COUNT = 98;

// Диапазон значений АЦП
inline constexpr std::uint16_t ADC_MIN = 0;
inline constexpr std::uint16_t ADC_MAX = 4095;

// Значения по умолчанию
inline constexpr std::uint16_t DEFAULT_ON_GREEN = 2600;
inline constexpr std::uint16_t DEFAULT_ON_RED = 1000;
inline constexpr std::uint16_t DEFAULT_OFF_GREEN = 2100;
inline constexpr std::uint16_t DEFAULT_OFF_RED = 2400;

} // namespace piano
```

### Перечисления для типов безопасности

```cpp
namespace piano {

/**
 * @brief Тип сенсора (датчика)
 */
enum class SensorType : std::uint8_t {
    Green = 0,  // Зелёный сенсор
    Red   = 1,  // Красный сенсор
    Count = 2
};

/**
 * @brief Режим работы клавиши
 */
enum class KeyMode : std::uint8_t {
    Hammer = 0, // Режим молоточка (on)
    Damper = 1, // Режим демпфера (off)
    Count  = 2
};

/**
 * @brief Точка срабатывания
 */
enum class TriggerPoint : std::uint8_t {
    On1  = 0,  // Первая точка включения
    On2  = 1,  // Вторая точка включения
    Off1 = 0,  // Первая точка выключения
    Off2 = 1,  // Вторая точка выключения
    Off3 = 2,  // Третья точка выключения
    CountOn = 2,
    CountOff = 3
};

/**
 * @brief Состояние калибровки
 */
enum class CalibrationStatus : std::uint8_t {
    NotCalibrated = 0,
    Calibrated    = 1,
    NeedsUpdate   = 2,
    Error         = 3
};

/**
 * @brief Адрес чипа G4 на шине
 */
class ChipAddress {
public:
    static constexpr std::uint8_t MIN = 1;
    static constexpr std::uint8_t MAX = 27;
    
    constexpr ChipAddress() noexcept : value_(0) {}
    constexpr explicit ChipAddress(std::uint8_t addr) : value_(addr) {
        static_assert(MIN <= MAX);
    }
    
    constexpr std::uint8_t value() const noexcept { return value_; }
    constexpr explicit operator std::uint8_t() const noexcept { return value_; }
    
    constexpr bool isValid() const noexcept { 
        return value_ >= MIN && value_ <= MAX; 
    }
    
    constexpr bool isOnChip() const noexcept {
        return value_ >= 1 && value_ <= 13;
    }
    
    constexpr bool isOffChip() const noexcept {
        return value_ >= 14 && value_ <= 23;
    }
    
    constexpr ChipAddress& operator++() noexcept { 
        if (value_ < MAX) ++value_; 
        return *this; 
    }
    
    constexpr ChipAddress& operator--() noexcept { 
        if (value_ > MIN) --value_; 
        return *this; 
    }
    
    constexpr ChipAddress operator++(int) noexcept { 
        ChipAddress tmp(*this); 
        ++(*this); 
        return tmp; 
    }
    
    constexpr ChipAddress operator--(int) noexcept { 
        ChipAddress tmp(*this); 
        --(*this); 
        return tmp; 
    }
    
private:
    std::uint8_t value_;
};

} // namespace piano

// Операторы сравнения
constexpr bool operator==(piano::ChipAddress a, piano::ChipAddress b) noexcept {
    return a.value() == b.value();
}
constexpr bool operator!=(piano::ChipAddress a, piano::ChipAddress b) noexcept {
    return a.value() != b.value();
}
constexpr bool operator<(piano::ChipAddress a, piano::ChipAddress b) noexcept {
    return a.value() < b.value();
}
constexpr bool operator<=(piano::ChipAddress a, piano::ChipAddress b) noexcept {
    return a.value() <= b.value();
}
constexpr bool operator>(piano::ChipAddress a, piano::ChipAddress b) noexcept {
    return a.value() > b.value();
}
constexpr bool operator>=(piano::ChipAddress a, piano::ChipAddress b) noexcept {
    return a.value() >= b.value();
}
```

### Класс ноты

```cpp
namespace piano {

/**
 * @brief Номер ноты MIDI (0-127)
 */
class MidiNote {
public:
    static constexpr std::uint8_t MIN = 0;
    static constexpr std::uint8_t MAX = 127;
    static constexpr std::uint8_t PIANO_MIN = 21;    // A0
    static constexpr std::uint8_t PIANO_MAX = 108;    // C8
    
    constexpr MidiNote() noexcept : value_(0) {}
    constexpr explicit MidiNote(std::uint8_t note) : value_(note) {}
    
    constexpr std::uint8_t value() const noexcept { return value_; }
    constexpr explicit operator std::uint8_t() const noexcept { return value_; }
    
    constexpr bool isValid() const noexcept {
        return value_ >= MIN && value_ <= MAX;
    }
    
    constexpr bool isPianoNote() const noexcept {
        return value_ >= PIANO_MIN && value_ <= PIANO_MAX;
    }
    
    // Проверка, белая или чёрная клавиша
    constexpr bool isWhiteKey() const noexcept {
        constexpr std::array<bool, 12> whiteKeys = {
            true, false, true, true, false, 
            true, false, true, true, false, true, true
        };
        return whiteKeys[value_ % 12];
    }
    
private:
    std::uint8_t value_;
};

} // namespace piano
```

### Индекс массива для дисплея

```cpp
namespace piano {

/**
 * @brief Индекс в массиве для отображения на дисплее (0-195)
 */
class DisplayIndex {
public:
    static constexpr std::size_t MIN = 0;
    static constexpr std::size_t MAX = 195;
    
    constexpr DisplayIndex() noexcept : value_(0) {}
    constexpr explicit DisplayIndex(std::size_t idx) : value_(idx) {}
    
    constexpr std::size_t value() const noexcept { return value_; }
    constexpr explicit operator std::size_t() const noexcept { return value_; }
    
    constexpr bool isValid() const noexcept {
        return value_ <= MAX;
    }
    
    // Получить номер чипа (0-26)
    constexpr std::size_t chipIndex() const noexcept {
        return value_ / COMPARATORS_PER_CHIP;
    }
    
    // Получить номер компаратора на чипе (0-6)
    constexpr std::size_t comparatorOnChip() const noexcept {
        return value_ % COMPARATORS_PER_CHIP;
    }
    
    // Это режим ON (hammer) или OFF (damper)?
    constexpr KeyMode keyMode() const noexcept {
        return value_ < ON_NOTES_COUNT ? KeyMode::Hammer : KeyMode::Damper;
    }
    
    // Получить смещение для режима OFF (98 или 0)
    constexpr std::size_t modeOffset() const noexcept {
        return value_ >= ON_NOTES_COUNT ? ON_NOTES_COUNT : 0;
    }
    
    // Получить локальный индекс внутри режима
    constexpr std::size_t localIndex() const noexcept {
        return value_ % ON_NOTES_COUNT;
    }
    
    // Конвертация в номер ноты MIDI (с учётом сдвига)
    MidiNote toMidiNote() const noexcept;
    
    // Статический конструктор из номера чипа и компаратора
    static constexpr DisplayIndex fromChipAndComparator(
        std::size_t chipIdx, 
        std::size_t compIdx,
        KeyMode mode
    ) noexcept {
        return DisplayIndex(
            chipIdx * COMPARATORS_PER_CHIP + compIdx + 
            (mode == KeyMode::Damper ? ON_NOTES_COUNT : 0)
        );
    }
    
private:
    std::size_t value_;
};

} // namespace piano
```

### Структура калибровки одного сенсора

```cpp
namespace piano {

/**
 * @brief Значение калибровки одного сенсора
 */
struct SensorCalibration {
    std::uint16_t on_threshold = DEFAULT_ON_GREEN;
    std::uint16_t off_threshold = DEFAULT_OFF_GREEN;
    
    constexpr bool operator==(const SensorCalibration& other) const noexcept {
        return on_threshold == other.on_threshold && 
               off_threshold == other.off_threshold;
    }
    
    constexpr bool operator!=(const SensorCalibration& other) const noexcept {
        return !(*this == other);
    }
    
    // Проверка валидности значений
    constexpr bool isValid() const noexcept {
        return on_threshold <= ADC_MAX && off_threshold <= ADC_MAX;
    }
    
    // Сброс к значениям по умолчанию
    constexpr void reset(SensorType type) noexcept {
        switch (type) {
            case SensorType::Green:
                on_threshold = DEFAULT_ON_GREEN;
                off_threshold = DEFAULT_OFF_GREEN;
                break;
            case SensorType::Red:
                on_threshold = DEFAULT_ON_RED;
                off_threshold = DEFAULT_OFF_RED;
                break;
            default:
                break;
        }
    }
};

/**
 * @brief Калибровка одного компаратора (оба сенсора)
 */
struct ComparatorCalibration {
    std::array<SensorCalibration, 2> sensors{}; // [0] = Green, [1] = Red
    
    constexpr SensorCalibration& green() noexcept { return sensors[0]; }
    constexpr const SensorCalibration& green() const noexcept { return sensors[0]; }
    
    constexpr SensorCalibration& red() noexcept { return sensors[1]; }
    constexpr const SensorCalibration& red() const noexcept { return sensors[1]; }
    
    constexpr SensorCalibration& sensor(SensorType type) noexcept {
        return sensors[static_cast<std::size_t>(type)];
    }
    
    constexpr const SensorCalibration& sensor(SensorType type) const noexcept {
        return sensors[static_cast<std::size_t>(type)];
    }
    
    constexpr bool operator==(const ComparatorCalibration& other) const noexcept {
        return sensors == other.sensors;
    }
    
    constexpr bool operator!=(const ComparatorCalibration& other) const noexcept {
        return !(*this == other);
    }
};

/**
 * @brief Получить ссылку на калибровку по индексу режима
 */
constexpr ComparatorCalibration& getCalibrationByMode(
    std::array<ComparatorCalibration, ON_NOTES_COUNT>& data,
    KeyMode mode
) noexcept {
    // Для режима OFF используем ту же структуру (смещение обрабатывается на уровне доступа)
    return data[0]; // Заглушка - реальная логика сложнее
}

} // namespace piano
```

### Основной класс управления калибровкой

```cpp
namespace piano {

/**
 * @brief Основной класс хранения и управления калибровкой
 */
class CalibrationManager {
public:
    // Типы для безопасного доступа
    using ChipIndex = std::size_t;           // 0-26 (индекс чипа в массиве)
    using ComparatorIndex = std::size_t;     // 0-6 (индекс компаратора на чипе)
    using NoteIndex = std::size_t;           // 0-196 (индекс ноты)
    
    CalibrationManager() noexcept {
        resetToDefaults();
    }
    
    // Сброс всех значений к умолчанию
    void resetToDefaults() noexcept {
        for (auto& comp : data_) {
            comp.green().reset(SensorType::Green);
            comp.red().reset(SensorType::Red);
        }
    }
    
    // ============ Доступ по номеру чипа и компаратора ============
    
    ComparatorCalibration& getComparator(ChipAddress chip, ComparatorIndex compIdx) noexcept {
        return data_[chip.value()][compIdx];
    }
    
    const ComparatorCalibration& getComparator(ChipAddress chip, ComparatorIndex compIdx) const noexcept {
        return data_[chip.value()][compIdx];
    }
    
    // ============ Доступ по индексу дисплея ============
    
    ComparatorCalibration& getByDisplayIndex(DisplayIndex idx) noexcept {
        return data_[idx.chipIndex()][idx.comparatorOnChip()];
    }
    
    const ComparatorCalibration& getByDisplayIndex(DisplayIndex idx) const noexcept {
        return data_[idx.chipIndex()][idx.comparatorOnChip()];
    }
    
    // ============ Доступ по номеру ноты MIDI ============
    
    // Примечание: mapping MIDI note -> calibration index зависит от конкретной клавиатуры
    // Здесь используется упрощённая схема
    
    ComparatorCalibration& getByMidiNote(MidiNote note) noexcept;
    const ComparatorCalibration& getByMidiNote(MidiNote note) const noexcept;
    
    // ============ Группировка по режимам ============
    
    std::span<ComparatorCalibration, ON_NOTES_COUNT> onMode() noexcept {
        return std::span<ComparatorCalibration, ON_NOTES_COUNT>(
            data_.data(), 
            ON_NOTES_COUNT
        );
    }
    
    std::span<const ComparatorCalibration, ON_NOTES_COUNT> onMode() const noexcept {
        return std::span<const ComparatorCalibration, ON_NOTES_COUNT>(
            data_.data(), 
            ON_NOTES_COUNT
        );
    }
    
    std::span<ComparatorCalibration, OFF_NOTES_COUNT> offMode() noexcept {
        return std::span<ComparatorCalibration, OFF_NOTES_COUNT>(
            data_.data() + ON_NOTES_COUNT, 
            OFF_NOTES_COUNT
        );
    }
    
    std::span<const ComparatorCalibration, OFF_NOTES_COUNT> offMode() const noexcept {
        return std::span<const ComparatorCalibration, OFF_NOTES_COUNT>(
            data_.data() + ON_NOTES_COUNT, 
            OFF_NOTES_COUNT
        );
    }
    
    // ============ Итераторы ============
    
    auto begin() noexcept { return data_.begin(); }
    auto end() noexcept { return data_.end(); }
    auto begin() const noexcept { return data_.begin(); }
    auto end() const noexcept { return data_.end(); }
    
    // ============ Размеры ============
    
    static constexpr std::size_t size() noexcept { return TOTAL_NOTES; }
    static constexpr std::size_t chipCount() noexcept { return MAX_CHIPS; }
    static constexpr std::size_t comparatorsPerChip() noexcept { return COMPARATORS_PER_CHIP; }
    
    // ============ Проверка изменений ============
    
    bool hasChanges() const noexcept {
        return dirty_ != 0;
    }
    
    void markClean() noexcept {
        dirty_ = false;
    }
    
    void markDirty() noexcept {
        dirty_ = true;
    }
    
    // ============ Сериализация для Flash ============
    
    std::span<const std::uint8_t> serializeToFlash() const noexcept {
        return std::as_bytes(std::span(data_));
    }
    
    bool deserializeFromFlash(std::span<const std::uint8_t> data) noexcept;
    
    // ============ UART коммуникация ============
    
    // Формат сообщения для G4:
    // [command(1)] [chip_addr(1)] [comp_idx(1)] [sensor_type(1)] [value_hi(1)] [value_lo(1)]
    // Всего 6 байт
    
    struct UartMessage {
        std::uint8_t command;
        std::uint8_t chip_address;
        std::uint8_t comparator_index;
        std::uint8_t sensor_type;
        std::uint8_t value_high;
        std::uint8_t value_low;
        
        static constexpr std::size_t size() noexcept { return 6; }
        
        // Собрать из данных
        static UartMessage makeRead(
            std::uint8_t chip_addr,
            std::uint8_t comp_idx,
            SensorType sensor
        ) noexcept;
        
        static UartMessage makeWrite(
            std::uint8_t chip_addr,
            std::uint8_t comp_idx,
            SensorType sensor,
            std::uint16_t value
        ) noexcept;
        
        // Парсинг принятого сообщения
        bool parse(const std::uint8_t* data) noexcept;
        
        // Упаковка для отправки
        void pack(std::uint8_t* out) const noexcept;
        
        std::uint16_t value() const noexcept {
            return (static_cast<std::uint16_t>(value_high) << 8) | value_low;
        }
    };
    
    // Отправить значение на G4
    bool sendToChip(
        ChipAddress chip,
        ComparatorIndex comp,
        SensorType sensor,
        std::uint16_t value,
        auto&& sendFunc
    ) noexcept;
    
    // Прочитать значение с G4
    bool readFromChip(
        ChipAddress chip,
        ComparatorIndex comp,
        SensorType sensor,
        std::uint16_t& value,
        auto&& receiveFunc
    ) noexcept;
    
private:
    // Хранение данных: [chip_index][comparator_index]
    // Всегда в памяти: 27 чипов × 7 компараторов = 189 элементов
    // Но используются только первые 14 чипов (98 нот) × 7 = 686 компараторов
    // Для каждого компаратора: 2 сенсора × 2 порога (on/off) = 4 значения × 2 байта = 8 байт
    // Итого: 686 × 8 = 5488 байт (~5.4 KB)
    alignas(4) std::array<std::array<ComparatorCalibration, COMPARATORS_PER_CHIP>, MAX_CHIPS> data_;
    
    bool dirty_ = false;
    
    // Проверка, что чип и компаратор в допустимых пределах
    static constexpr bool isValidChip(ChipIndex chip) noexcept {
        return chip < MAX_CHIPS;
    }
    
    static constexpr bool isValidComparator(ComparatorIndex comp) noexcept {
        return comp < COMPARATORS_PER_CHIP;
    }
};

} // namespace piano
```

### Реализация методов конвертации

```cpp
namespace piano {

// Статические константы для mapping
inline constexpr std::int8_t NOTE_OFFSETS[196] = {
    // Индексы для коррекции MIDI номера
    // Эти значения зависят от конкретной клавиатуры
    [0 ... 53] = 14,    // 0-54: +14
    [54 ... 95] = 13,   // 55-95: +13
    [96 ... 145] = -77, // 96-145: -77
    [146 ... 195] = -78 // 146+: -78
};

MidiNote DisplayIndex::toMidiNote() const noexcept {
    std::uint8_t baseNote = static_cast<std::uint8_t>(value_);
    std::int8_t offset = NOTE_OFFSETS[value_];
    std::int16_t result = static_cast<std::int16_t>(baseNote) + offset;
    
    if (result < 0) result = 0;
    if (result > 127) result = 127;
    
    return MidiNote(static_cast<std::uint8_t>(result));
}

ComparatorCalibration& CalibrationManager::getByMidiNote(MidiNote note) noexcept {
    // Здесь должна быть обратная логика mapping
    // Для примера - используем прямой индекс
    if (note.value() < TOTAL_NOTES) {
        std::size_t idx = note.value();
        return data_[idx / COMPARATORS_PER_CHIP][idx % COMPARATORS_PER_CHIP];
    }
    return data_[0][0]; // Заглушка для невалидных значений
}

const ComparatorCalibration& CalibrationManager::getByMidiNote(MidiNote note) const noexcept {
    if (note.value() < TOTAL_NOTES) {
        std::size_t idx = note.value();
        return data_[idx / COMPARATORS_PER_CHIP][idx % COMPARATORS_PER_CHIP];
    }
    return data_[0][0];
}

bool CalibrationManager::deserializeFromFlash(
    std::span<const std::uint8_t> data
) noexcept {
    if (data.size() < sizeof(data_)) {
        return false;
    }
    
    // Копируем данные с проверкой выравнивания
    const auto* src = reinterpret_cast<const std::uint8_t*>(data.data());
    auto* dst = reinterpret_cast<std::uint8_t*>(data_.data());
    
    for (std::size_t i = 0; i < sizeof(data_); ++i) {
        dst[i] = src[i];
    }
    
    dirty_ = false;
    return true;
}

UartMessage UartMessage::makeRead(
    std::uint8_t chip_addr,
    std::uint8_t comp_idx,
    SensorType sensor
) noexcept {
    UartMessage msg;
    msg.command = 0x03; // read_comp_value
    msg.chip_address = chip_addr;
    msg.comparator_index = comp_idx;
    msg.sensor_type = static_cast<std::uint8_t>(sensor);
    msg.value_high = 0;
    msg.value_low = 0;
    return msg;
}

UartMessage UartMessage::makeWrite(
    std::uint8_t chip_addr,
    std::uint8_t comp_idx,
    SensorType sensor,
    std::uint16_t value
) noexcept {
    UartMessage msg;
    msg.command = 0x04; // set_comp_value
    msg.chip_address = chip_addr;
    msg.comparator_index = comp_idx;
    msg.sensor_type = static_cast<std::uint8_t>(sensor);
    msg.value_high = static_cast<std::uint8_t>((value >> 8) & 0xFF);
    msg.value_low = static_cast<std::uint8_t>(value & 0xFF);
    return msg;
}

bool UartMessage::parse(const std::uint8_t* data) noexcept {
    command = data[0];
    chip_address = data[1];
    comparator_index = data[2];
    sensor_type = data[3];
    value_high = data[4];
    value_low = data[5];
    return true;
}

void UartMessage::pack(std::uint8_t* out) const noexcept {
    out[0] = command;
    out[1] = chip_address;
    out[2] = comparator_index;
    out[3] = sensor_type;
    out[4] = value_high;
    out[5] = value_low;
}

} // namespace piano
```

---

## Использование в UART коммуникации

### Формат сообщений

Текущий формат (5 байт) сохраняется с точностью до 1 байта:

```cpp
namespace uart {

// Размеры буферов
inline constexpr std::size_t TX_BUFFER_SIZE = 5;
inline constexpr std::size_t RX_BUFFER_SIZE = 5;

/**
 * @brief Формат передаваемого сообщения (совместим с текущим)
 */
struct Message {
    std::uint8_t command;    // 0: команда
    std::uint8_t compN;      // 1: номер компаратора
    std::uint8_t dot;        // 2: тип сенсора (0=green, 1=red)
    std::uint8_t value_hi;   // 3: старший байт значения
    std::uint8_t value_lo;   // 4: младший байт значения
    
    static constexpr std::size_t size() noexcept { return TX_BUFFER_SIZE; }
};

/**
 * @brief Парсинг входящего ответа от G4
 */
struct Response {
    std::uint8_t chip_addr;  // 0: адрес чипа (для проверки)
    std::uint8_t compN;      // 1: номер компаратора
    std::uint8_t dot;        // 2: тип сенсора
    std::uint8_t value_hi;   // 3: старший байт
    std::uint8_t value_lo;   // 4: младший байт
    
    static constexpr std::size_t size() noexcept { return RX_BUFFER_SIZE; }
    
    std::uint16_t value() const noexcept {
        return (static_cast<std::uint16_t>(value_hi) << 8) | value_lo;
    }
};

} // namespace uart
```

### Класс для работы с UART

```cpp
namespace uart {

/**
 * @brief Абстракция для отправки команд G4
 */
class G4Driver {
public:
    G4Driver(
        auto&& sendAddress,
        auto&& sendData,
        auto&& receiveData
    ) : sendAddress_(sendAddress),
        sendData_(sendData),
        receiveData_(receiveData) {}
    
    // Отправить команду на чип
    bool sendCommand(
        piano::ChipAddress chip,
        const Message& msg,
        std::chrono::microseconds timeout = std::chrono::milliseconds(10)
    ) {
        // 1. Отправляем адрес (старший бит = 1)
        sendAddress_(chip.value() | 0x100);
        
        // 2. Ждём
        pause(3);
        
        // 3. Отправляем данные
        sendData_(msg);
        
        // 4. Получаем ответ
        return receiveData_(response_, timeout);
    }
    
    // Чтение значения с компаратора
    bool readComparator(
        piano::ChipAddress chip,
        piano::ComparatorIndex compIdx,
        piano::SensorType sensor,
        std::uint16_t& value
    ) {
        Message msg;
        msg.command = 0x03; // read_comp_value
        msg.compN = static_cast<std::uint8_t>(compIdx);
        msg.dot = static_cast<std::uint8_t>(sensor);
        msg.value_hi = 0;
        msg.value_lo = 0;
        
        if (!sendCommand(chip, msg)) {
            return false;
        }
        
        // Проверяем, что ответ от того же чипа
        if (response_.chip_addr != chip.value()) {
            return false;
        }
        
        value = response_.value();
        return true;
    }
    
    // Запись значения в компаратор
    bool writeComparator(
        piano::ChipAddress chip,
        piano::ComparatorIndex compIdx,
        piano::SensorType sensor,
        std::uint16_t value
    ) {
        Message msg;
        msg.command = 0x04; // set_comp_value
        msg.compN = static_cast<std::uint8_t>(compIdx);
        msg.dot = static_cast<std::uint8_t>(sensor);
        msg.value_hi = static_cast<std::uint8_t>((value >> 8) & 0xFF);
        msg.value_lo = static_cast<std::uint8_t>(value & 0xFF);
        
        if (!sendCommand(chip, msg)) {
            return false;
        }
        
        // Проверяем записанное значение
        return response_.value() == value;
    }
    
    // Синхронизация таймера
    bool syncTimer(piano::ChipAddress chip) {
        Message msg;
        msg.command = 0x01; // sync_timer
        msg.compN = 0;
        msg.dot = 0;
        msg.value_hi = 0;
        msg.value_lo = 0;
        
        return sendCommand(chip, msg);
    }
    
    // Пакетная передача всех калибровок на чип
    bool uploadCalibration(
        piano::ChipAddress chip,
        std::span<const piano::ComparatorCalibration, 7> calibs
    ) {
        for (std::size_t i = 0; i < 7; ++i) {
            // Green ON
            if (!writeComparator(chip, i, piano::SensorType::Green, 
                    calibs[i].green().on_threshold)) {
                return false;
            }
            // Red ON
            if (!writeComparator(chip, i, piano::SensorType::Red,
                    calibs[i].red().on_threshold)) {
                return false;
            }
        }
        return true;
    }
    
private:
    Response response_;
    
    std::function<void(std::uint8_t)> sendAddress_;
    std::function<void(const Message&)> sendData_;
    std::function<bool(Response&, std::chrono::microseconds)> receiveData_;
};

} // namespace uart
```

---

## LVGL интеграция

```cpp
namespace display {

/**
 * @brief Интеграция с LVGL Chart
 */
class CalibrationChart {
public:
    CalibrationChart(
        lv_obj_t* chart,
        lv_chart_series_t* green_series,
        lv_chart_series_t* red_series,
        lv_chart_series_t* calib_series
    ) : chart_(chart),
        green_series_(green_series),
        red_series_(red_series),
        calib_series_(calib_series) {}
    
    // Обновить данные из калибровки
    void update(const piano::CalibrationManager& calib) noexcept {
        const auto& onData = calib.onMode();
        const auto& offData = calib.offMode();
        
        // Обновляем зелёный сенсор
        for (std::size_t i = 0; i < onData.size(); ++i) {
            green_series_->y[i] = static_cast<lv_coord_t>(onData[i].green().on_threshold);
        }
        
        // Обновляем красный сенсор
        for (std::size_t i = 0; i < onData.size(); ++i) {
            red_series_->y[i] = static_cast<lv_coord_t>(onData[i].red().on_threshold);
        }
        
        // Обновляем онлайн калибровку
        // Это значение должно обновляться в реальном времени из прерывания
        for (std::size_t i = 0; i < onData.size(); ++i) {
            calib_series_->y[i] = static_cast<lv_coord_t>(online_calib_[i]);
        }
        
        lv_chart_refresh(chart_);
    }
    
    // Установить онлайн значение
    void setOnlineValue(std::size_t index, std::uint16_t value) noexcept {
        if (index < online_calib_.size()) {
            online_calib_[index] = value;
        }
    }
    
    // Установить позицию курсора
    void setCursor(std::size_t index) noexcept {
        if (index < online_calib_.size()) {
            lv_chart_set_cursor_point(chart_, cursor_, green_series_, 
                static_cast<lv_coord_t>(index));
        }
    }
    
private:
    lv_obj_t* chart_;
    lv_chart_series_t* green_series_;
    lv_chart_series_t* red_series_;
    lv_chart_series_t* calib_series_;
    lv_chart_cursor_t* cursor_;
    
    std::array<std::uint16_t, piano::ON_NOTES_COUNT> online_calib_{};
    std::array<std::uint16_t, piano::OFF_NOTES_COUNT> online_calib_off_{};
};

} // namespace display
```

---

## Примеры использования

### До и после (сравнение кода)

**До (текущий код):**
```cpp
// Получение адреса и номера компаратора
uint8_t adress = cursor / 7;
uint8_t compN = cursor % 7;

// Отправка данных
sender(command::set_comp_value, adress, compN, 0, compsCHART_0[cursor]);

// Обновление дисплея
compsCHART_0[(adress * 7) + i] = convert_8_16(a_, b_);
```

**После (новый код):**
```cpp
// Использование типобезопасных индексов
auto displayIdx = piano::DisplayIndex{cursor};
auto chip = piano::ChipAddress{displayIdx.chipIndex() + 1};
auto compIdx = displayIdx.comparatorOnChip();

// Отправка данных
auto msg = piano::UartMessage::makeWrite(
    chip.value(), 
    static_cast<std::uint8_t>(compIdx),
    piano::SensorType::Green,
    calib.getComparator(chip, compIdx).green().on_threshold
);

// Или ещё проще - через менеджер
calib.sendToChip(chip, compIdx, piano::SensorType::Green,
    calib.getComparator(chip, compIdx).green().on_threshold,
    sendFunction);
```

---

## Рекомендации по улучшению

### 1. Замена std::string на строковые буферы

```cpp
// Вместо std::string использовать char[] или static_string
class StaticString {
    static constexpr std::size_t MAX_SIZE = 64;
    char buffer_[MAX_SIZE];
    std::size_t size_ = 0;
    
public:
    constexpr void assign(const char* str) noexcept {
        size_ = 0;
        while (str[size_] && size_ < MAX_SIZE - 1) {
            buffer_[size_] = str[size_];
            ++size_;
        }
        buffer_[size_] = '\0';
    }
    
    constexpr const char* c_str() const noexcept { return buffer_; }
    constexpr std::size_t size() const noexcept { return size_; }
};
```

### 2. Оптимизация прерывания MIDI

Текущее прерывание обрабатывает слишком много данных. Рекомендуется:

```cpp
// Обработка в прерывании - только принятие данных
void DMA_IRQHandler() {
    // Копируем данные в кольцевой буфер
    volatile uint8_t rxBuffer[4];
    rxBuffer[0] = UART5->RDR;
    rxBuffer[1] = UART5->RDR;
    rxBuffer[2] = UART5->RDR;
    rxBuffer[3] = UART5->RDR;
    
    // Только ставим флаг, что данные готовы
    midiDataReady_ = true;
    
    // Обработка - в main loop
    if (midiDataReady_) {
        processMidiData(rxBuffer);
        midiDataReady_ = false;
    }
}
```

### 3. Использование кольцевого буфера для калибровки

```cpp
template<typename T, std::size_t N>
class RingBuffer {
    std::array<T, N> buffer_;
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
    std::size_t count_ = 0;
    
public:
    bool push(const T& value) {
        if (count_ == N) return false;
        buffer_[tail_] = value;
        tail_ = (tail_ + 1) % N;
        ++count_;
        return true;
    }
    
    std::optional<T> pop() {
        if (count_ == 0) return std::nullopt;
        T value = buffer_[head_];
        head_ = (head_ + 1) % N;
        --count_;
        return value;
    }
    
    std::size_t size() const noexcept { return count_; }
    bool empty() const noexcept { return count_ == 0; }
};
```

### 4. Кэширование вычисляемых значений

```cpp
// Вместо recalc каждый раз
class CalibrationCache {
    piano::CalibrationManager& calib_;
    
    // Кэшированные значения
    std::uint16_t min_green_on_ = 0;
    std::uint16_t max_green_on_ = ADC_MAX;
    std::uint16_t min_red_on_ = 0;
    std::uint16_t max_red_on_ = ADC_MAX;
    
    bool valid_ = false;
    
public:
    void invalidate() { valid_ = false; }
    
    void recompute() {
        if (valid_) return;
        
        min_green_on_ = ADC_MAX;
        max_green_on_ = 0;
        
        for (const auto& comp : calib_.onMode()) {
            min_green_on_ = std::min(min_green_on_, comp.green().on_threshold);
            max_green_on_ = std::max(max_green_on_, comp.green().on_threshold);
        }
        
        valid_ = true;
    }
    
    std::uint16_t minGreenOn() { recompute(); return min_green_on_; }
    std::uint16_t maxGreenOn() { recompute(); return max_green_on_; }
};
```

---

## Итоговый размер данных

| Параметр | Значение |
|----------|----------|
| Количество чипов | 27 |
| Компараторов на чип | 7 |
| Сенсоров на компаратор | 2 |
| Значений на сенсор (on/off) | 2 |
| Байт на значение | 2 |
| Общий размер калибровки | 27 × 7 × 2 × 2 × 2 = 1512 байт |
| С запасом и выравниванием | ~2 КБ |
| Буфер онлайн-калибровки | 196 × 2 = 392 байт |
| **Итого RAM** | **~3 КБ** |

---

## Заключение

Предложенная структура данных обеспечивает:

1. **Типобезопасность**: компилятор предотвращает ошибки типов
2. **Удобство доступа**: единый API для всех операций
3. **Простоту конвертации**: встроенные методы для всех преобразований
4. **Совместимость**: сохранён формат UART сообщений (5 байт)
5. **Производительность**: constexpr методы, минимальные проверки
6. **Безопасность для embedded**: нет динамической памяти, std::string

Рекомендуется постепенный рефакторинг с созданием адаптеров для старого кода.
