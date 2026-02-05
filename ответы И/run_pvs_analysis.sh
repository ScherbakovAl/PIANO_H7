#!/bin/bash
# Скрипт для анализа проекта Piano_H7 с помощью PVS-Studio

set -e  # Остановить при ошибке

PROJECT_DIR="/home/sche/Documents/programming/Piano/PIANO_H7"
BUILD_DIR="$PROJECT_DIR/build"
REPORT_DIR="$PROJECT_DIR/pvs-report"

echo "========================================="
echo "PVS-Studio анализ проекта Piano_H7"
echo "========================================="
echo ""

# Проверка установки PVS-Studio
if ! command -v pvs-studio-analyzer &> /dev/null; then
    echo "ОШИБКА: PVS-Studio не установлен!"
    echo ""
    echo "Установите PVS-Studio:"
    echo "  wget -q -O - https://files.pvs-studio.com/etc/pubkey.txt | sudo apt-key add -"
    echo "  sudo wget -O /etc/apt/sources.list.d/viva64.list https://files.pvs-studio.com/etc/viva64.list"
    echo "  sudo apt update"
    echo "  sudo apt install pvs-studio"
    echo ""
    echo "Или используйте альтернативу - Cppcheck:"
    echo "  sudo apt install cppcheck"
    echo "  cppcheck --enable=all --inconclusive Piano_H7/"
    exit 1
fi

cd "$PROJECT_DIR"

# Шаг 1: Генерация compile_commands.json
echo "[1/4] Генерация compile_commands.json..."
if [ -d "$BUILD_DIR" ]; then
    echo "  Очистка старой сборки..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Попытка использовать preset, если не получится - обычный cmake
if cmake --preset=default -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 2>/dev/null; then
    echo "  ✓ Использован preset"
else
    echo "  Preset не найден, использую обычный cmake..."
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
fi

if [ ! -f "compile_commands.json" ]; then
    echo "  ОШИБКА: compile_commands.json не создан!"
    exit 1
fi

echo "  ✓ compile_commands.json создан"
cd "$PROJECT_DIR"

# Шаг 2: Запуск анализа
echo ""
echo "[2/4] Запуск PVS-Studio анализа..."
echo "  Это может занять несколько минут..."

pvs-studio-analyzer analyze \
    -f "$BUILD_DIR/compile_commands.json" \
    -o pvs-studio.log \
    -e Drivers/ \
    -e lvgl/ \
    -e eez_proj/ \
    -e H7_dfu_bootloader/Drivers/ \
    -j$(nproc) \
    2>&1 | tee pvs-analysis.log

if [ ! -f "pvs-studio.log" ]; then
    echo "  ОШИБКА: Анализ не выполнен!"
    exit 1
fi

echo "  ✓ Анализ завершен"

# Шаг 3: Генерация отчетов
echo ""
echo "[3/4] Генерация отчетов..."

# HTML отчет
if [ -d "$REPORT_DIR" ]; then
    rm -rf "$REPORT_DIR"
fi

plog-converter -t html -o "$REPORT_DIR" pvs-studio.log
echo "  ✓ HTML отчет: $REPORT_DIR/index.html"

# Текстовый отчет
plog-converter -t tasklist -o pvs-report.txt pvs-studio.log
echo "  ✓ Текстовый отчет: pvs-report.txt"

# CSV отчет
plog-converter -t csv -o pvs-report.csv pvs-studio.log
echo "  ✓ CSV отчет: pvs-report.csv"

# Шаг 4: Показать краткую статистику
echo ""
echo "[4/4] Краткая статистика проблем:"
echo "========================================="

# Подсчет проблем по уровням
HIGH=$(plog-converter -t errorfile -a GA:1 -a 64:1 -a OP:1 pvs-studio.log 2>/dev/null | wc -l)
MEDIUM=$(plog-converter -t errorfile -a GA:2 -a 64:2 -a OP:2 pvs-studio.log 2>/dev/null | wc -l)
LOW=$(plog-converter -t errorfile -a GA:3 -a 64:3 -a OP:3 pvs-studio.log 2>/dev/null | wc -l)

echo "  Высокий уровень (High):   $HIGH"
echo "  Средний уровень (Medium): $MEDIUM"
echo "  Низкий уровень (Low):     $LOW"
echo ""

# Показать первые 20 проблем высокого уровня
echo "Первые проблемы высокого уровня:"
echo "-----------------------------------------"
plog-converter -t errorfile -a GA:1,2 -a 64:1,2 -a OP:1,2 pvs-studio.log 2>/dev/null | head -n 20

echo ""
echo "========================================="
echo "Анализ завершен!"
echo ""
echo "Отчеты сохранены в:"
echo "  HTML:  $REPORT_DIR/index.html"
echo "  TXT:   pvs-report.txt"
echo "  CSV:   pvs-report.csv"
echo ""
echo "Открыть HTML отчет:"
echo "  xdg-open $REPORT_DIR/index.html"
echo ""
echo "Просмотреть текстовый отчет:"
echo "  cat pvs-report.txt"
echo "========================================="
