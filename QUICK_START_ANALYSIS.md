# Быстрый старт: Анализ кода Piano_H7

## Вариант 1: PVS-Studio (коммерческий, мощный)

### Установка
```bash
wget -q -O - https://files.pvs-studio.com/etc/pubkey.txt | sudo apt-key add -
sudo wget -O /etc/apt/sources.list.d/viva64.list https://files.pvs-studio.com/etc/viva64.list
sudo apt update
sudo apt install pvs-studio
```

### Запуск анализа
```bash
cd /home/sche/Documents/programming/Piano/PIANO_H7
./run_pvs_analysis.sh
```

### Просмотр результатов
```bash
# HTML отчет (откроется в браузере)
xdg-open pvs-report/index.html

# Текстовый отчет
cat pvs-report.txt
```

---

## Вариант 2: Cppcheck (бесплатный, простой)

### Установка
```bash
sudo apt update
sudo apt install cppcheck
```

### Запуск анализа
```bash
cd /home/sche/Documents/programming/Piano/PIANO_H7
./run_cppcheck_analysis.sh
```

### Просмотр результатов
```bash
cat cppcheck-report.txt
```

---

## Вариант 3: Clang-Tidy (бесплатный, встроен в Clang)

### Установка
```bash
sudo apt install clang-tidy
```

### Быстрый анализ одного файла
```bash
clang-tidy Piano_H7/piano_h7.cpp -- \
    -std=c++20 \
    -I Core/Inc \
    -I Piano_H7 \
    -I Drivers/CMSIS/Include \
    -I Drivers/CMSIS/Device/ST/STM32H7xx/Include \
    -I Drivers/STM32H7xx_HAL_Driver/Inc
```

### Анализ всего проекта
```bash
# Сначала создать compile_commands.json
mkdir -p build && cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
cd ..

# Запустить анализ
run-clang-tidy -p build Piano_H7/
```

---

## Обнаруженная проблема в вашем коде

В файле [`piano_h7.cpp:861`](Piano_H7/piano_h7.cpp:861) есть ошибка приоритета операций:

### Текущий код (НЕПРАВИЛЬНО):
```cpp
r.a = (a & 0xff << 8) >> 8;
```

### Проблема:
Из-за приоритета операций это выполняется как:
```cpp
r.a = (a & (0xff << 8)) >> 8;  // 0xff << 8 = 0xff00
```

### Правильный вариант:
```cpp
r.a = (a >> 8) & 0xff;
```

Или если нужно именно так:
```cpp
r.a = ((a & (0xff << 8)) >> 8);
```

---

## Рекомендации

1. **Начните с Cppcheck** - он бесплатный и простой в использовании
2. **Затем попробуйте PVS-Studio** - более мощный, но требует лицензию
3. **Исправьте критические ошибки** - начните с высокого уровня
4. **Настройте CI/CD** - автоматический анализ при каждом коммите

---

## Что проверяют анализаторы

### PVS-Studio находит:
- ✓ Ошибки приоритета операций
- ✓ Утечки памяти
- ✓ Неинициализированные переменные
- ✓ Переполнение буфера
- ✓ Гонки данных (race conditions)
- ✓ Нарушения MISRA/AUTOSAR
- ✓ Уязвимости безопасности

### Cppcheck находит:
- ✓ Утечки памяти
- ✓ Неинициализированные переменные
- ✓ Выход за границы массива
- ✓ Неиспользуемые переменные
- ✓ Логические ошибки
- ✓ Проблемы производительности

### Clang-Tidy находит:
- ✓ Нарушения стиля кода
- ✓ Модернизация кода (C++11/14/17/20)
- ✓ Проблемы производительности
- ✓ Потенциальные баги
- ✓ Проблемы читаемости

---

## Пример вывода

После запуска анализа вы увидите что-то вроде:

```
Piano_H7/piano_h7.cpp:861: warning: operator precedence issue
  r.a = (a & 0xff << 8) >> 8;
        ^
Piano_H7/piano_h7.cpp:1299: warning: possible null pointer dereference
  if (UART4_Receive_timeout_10us() && rx_settings[0]) {
                                       ^
Piano_H7/piano_h7.cpp:626: performance: variable can be const
  int rxB = rx_data[0];
  ^
```

---

## Дополнительная информация

Подробное руководство: [`PVS_STUDIO_GUIDE.md`](PVS_STUDIO_GUIDE.md)

Скрипты для анализа:
- [`run_pvs_analysis.sh`](run_pvs_analysis.sh) - PVS-Studio
- [`run_cppcheck_analysis.sh`](run_cppcheck_analysis.sh) - Cppcheck
