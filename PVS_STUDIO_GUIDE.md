# Руководство по анализу кода с помощью PVS-Studio

## Что такое PVS-Studio?

PVS-Studio - это статический анализатор кода для C, C++, C# и Java, который обнаруживает:
- Ошибки в коде (потенциальные баги)
- Уязвимости безопасности
- Проблемы с производительностью
- Нарушения стандартов кодирования (MISRA, AUTOSAR и др.)
- Опечатки и логические ошибки

## Установка PVS-Studio на Linux

### Вариант 1: Установка из официального репозитория

```bash
# Скачать и установить ключ репозитория
wget -q -O - https://files.pvs-studio.com/etc/pubkey.txt | sudo apt-key add -

# Добавить репозиторий
sudo wget -O /etc/apt/sources.list.d/viva64.list \
    https://files.pvs-studio.com/etc/viva64.list

# Обновить список пакетов
sudo apt update

# Установить PVS-Studio
sudo apt install pvs-studio
```

### Вариант 2: Скачать .deb пакет вручную

```bash
# Перейти на сайт https://pvs-studio.com/en/pvs-studio/download-all/
# Скачать .deb пакет для вашей версии Linux
# Установить:
sudo dpkg -i pvs-studio-*.deb
sudo apt-get install -f  # Установить зависимости, если нужно
```

## Регистрация лицензии

PVS-Studio требует лицензию. Есть несколько вариантов:

### 1. Бесплатная лицензия для Open Source проектов
Если ваш проект открытый, можно получить бесплатную лицензию.

### 2. Триальная лицензия (30 дней)
```bash
pvs-studio-analyzer credentials <ИМЯ> <КЛЮЧ>
```

### 3. Использование без лицензии (с комментариями)
Добавьте в начало каждого файла комментарий:
```cpp
// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
```

## Анализ проекта на CMake

### Шаг 1: Генерация compile_commands.json

```bash
cd /home/sche/Documents/programming/Piano/PIANO_H7

# Очистить предыдущую сборку (опционально)
rm -rf build

# Создать директорию для сборки
mkdir -p build
cd build

# Сгенерировать файлы сборки с compile_commands.json
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

# Или если используете preset:
cmake --preset=default -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

### Шаг 2: Запуск анализа

```bash
# Вернуться в корень проекта
cd /home/sche/Documents/programming/Piano/PIANO_H7

# Запустить анализ
pvs-studio-analyzer analyze \
    -f build/compile_commands.json \
    -o pvs-studio.log \
    -j$(nproc)
```

Параметры:
- `-f` - путь к compile_commands.json
- `-o` - выходной файл с результатами
- `-j` - количество потоков ($(nproc) = все доступные ядра)

### Шаг 3: Конвертация результатов в читаемый формат

```bash
# HTML отчет (рекомендуется)
plog-converter -t html -o pvs-report pvs-studio.log

# Текстовый отчет
plog-converter -t tasklist -o pvs-report.txt pvs-studio.log

# CSV отчет
plog-converter -t csv -o pvs-report.csv pvs-studio.log

# Вывод в консоль с цветами
plog-converter -t errorfile pvs-studio.log
```

### Шаг 4: Просмотр результатов

```bash
# Открыть HTML отчет в браузере
xdg-open pvs-report/index.html

# Или просмотреть текстовый отчет
cat pvs-report.txt
```

## Анализ конкретных файлов

Если нужно проанализировать только определенные файлы:

```bash
# Анализ одного файла
pvs-studio-analyzer analyze \
    --source-file Piano_H7/piano_h7.cpp \
    --output-file piano_h7_analysis.log \
    --compiler arm-none-eabi-g++ \
    --preprocessor gcc \
    -I Core/Inc \
    -I Piano_H7 \
    -I Drivers/CMSIS/Include \
    -I Drivers/STM32H7xx_HAL_Driver/Inc \
    --std=c++20

# Конвертировать результат
plog-converter -t errorfile piano_h7_analysis.log
```

## Фильтрация результатов

### Исключить определенные директории

```bash
pvs-studio-analyzer analyze \
    -f build/compile_commands.json \
    -o pvs-studio.log \
    -e Drivers/ \
    -e lvgl/
```

### Показать только ошибки высокого уровня

```bash
plog-converter -t errorfile \
    -a GA:1,2 \
    -a 64:1,2 \
    -a OP:1,2 \
    pvs-studio.log
```

Уровни:
- 1 - Высокий (High) - критические ошибки
- 2 - Средний (Medium) - важные предупреждения
- 3 - Низкий (Low) - незначительные замечания

## Интеграция с VS Code

### Установка расширения

1. Откройте VS Code
2. Перейдите в Extensions (Ctrl+Shift+X)
3. Найдите "PVS-Studio"
4. Установите расширение

### Настройка

Создайте файл `.vscode/settings.json`:

```json
{
    "pvs-studio.analyzer.pathToAnalyzer": "/usr/bin/pvs-studio-analyzer",
    "pvs-studio.analyzer.pathToConverter": "/usr/bin/plog-converter",
    "pvs-studio.analyzer.compileCommandsPath": "${workspaceFolder}/build/compile_commands.json"
}
```

## Пример анализа вашего проекта

```bash
#!/bin/bash
# Скрипт для анализа проекта Piano_H7

PROJECT_DIR="/home/sche/Documents/programming/Piano/PIANO_H7"
cd "$PROJECT_DIR"

# 1. Генерация compile_commands.json
echo "Генерация compile_commands.json..."
mkdir -p build
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
cd ..

# 2. Запуск анализа
echo "Запуск PVS-Studio анализа..."
pvs-studio-analyzer analyze \
    -f build/compile_commands.json \
    -o pvs-studio.log \
    -e Drivers/ \
    -e lvgl/ \
    -j$(nproc)

# 3. Генерация отчетов
echo "Генерация отчетов..."
plog-converter -t html -o pvs-report pvs-studio.log
plog-converter -t tasklist -o pvs-report.txt pvs-studio.log

# 4. Показать краткую статистику
echo ""
echo "=== Краткая статистика ==="
plog-converter -t errorfile pvs-studio.log | head -n 50

echo ""
echo "Полный HTML отчет: pvs-report/index.html"
echo "Текстовый отчет: pvs-report.txt"
```

## Известные проблемы в вашем коде

Я уже заметил одну проблему в строке 861 файла `piano_h7.cpp`:

```cpp
// НЕПРАВИЛЬНО:
r.a = (a & 0xff << 8) >> 8;

// ПРАВИЛЬНО (приоритет операций):
r.a = (a & (0xff << 8)) >> 8;
// или
r.a = (a >> 8) & 0xff;
```

PVS-Studio обнаружит эту и другие подобные проблемы.

## Полезные ссылки

- Официальный сайт: https://pvs-studio.com
- Документация: https://pvs-studio.com/en/docs/
- Примеры использования: https://pvs-studio.com/en/blog/
- GitHub: https://github.com/viva64/pvs-studio-cmake-examples

## Альтернативные инструменты статического анализа

Если PVS-Studio не подходит, можно использовать:

1. **Cppcheck** (бесплатный)
   ```bash
   sudo apt install cppcheck
   cppcheck --enable=all --inconclusive Piano_H7/
   ```

2. **Clang Static Analyzer** (бесплатный)
   ```bash
   scan-build cmake ..
   scan-build make
   ```

3. **Clang-Tidy** (бесплатный)
   ```bash
   clang-tidy Piano_H7/piano_h7.cpp -- -std=c++20 -I...
   ```

## Рекомендации

1. Запускайте анализ регулярно (например, перед каждым коммитом)
2. Исправляйте ошибки высокого уровня в первую очередь
3. Настройте CI/CD для автоматического анализа
4. Используйте файл `.PVS-Studio.cfg` для настройки исключений
5. Добавьте комментарии для подавления ложных срабатываний:
   ```cpp
   // -V::1234  // Подавить предупреждение V1234
   ```
