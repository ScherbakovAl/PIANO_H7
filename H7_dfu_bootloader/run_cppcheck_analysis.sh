#!/bin/bash
# Скрипт для анализа проекта Piano_H7 с помощью Cppcheck (бесплатная альтернатива)

set -e

PROJECT_DIR="/home/sche/Documents/programming/Piano/PIANO_H7/H7_dfu_bootloader"

echo "========================================="
echo "Cppcheck анализ проекта Piano_H7"
echo "========================================="
echo ""

# Проверка установки Cppcheck
if ! command -v cppcheck &> /dev/null; then
    echo "Cppcheck не установлен. Установка..."
    sudo apt update
    sudo apt install -y cppcheck
fi

cd "$PROJECT_DIR"

echo "[1/2] Запуск анализа Cppcheck..."
echo "  Это может занять несколько минут..."
echo ""

# Запуск Cppcheck с подробными настройками
# Анализируем только исходные файлы проекта (Core, USB_DEVICE, Middlewares)
# Исключаем только сами исходники HAL драйверов (они генерируются ST)
cppcheck \
    --enable=all \
    --inconclusive \
    --std=c++11 \
    --platform=unix64 \
    --suppress=missingIncludeSystem \
    --suppress=unmatchedSuppression \
    --inline-suppr \
    -I Core/Inc \
    -I Drivers/CMSIS/Include \
    -I Drivers/CMSIS/Device/ST/STM32H7xx/Include \
    -I Drivers/STM32H7xx_HAL_Driver/Inc \
    --xml \
    --xml-version=2 \
    Core/ \
    USB_DEVICE/ \
    Middlewares/ \
    2> cppcheck-report.xml

echo ""
echo "[2/2] Генерация отчетов..."

# Текстовый отчет
cppcheck \
    --enable=all \
    --inconclusive \
    --std=c++11 \
    --platform=unix64 \
    --suppress=missingIncludeSystem \
    --suppress=unmatchedSuppression \
    --inline-suppr \
    -I Core/Inc \
    -I Drivers/CMSIS/Include \
    -I Drivers/CMSIS/Device/ST/STM32H7xx/Include \
    -I Drivers/STM32H7xx_HAL_Driver/Inc \
    Core/ \
    USB_DEVICE/ \
    Middlewares/ \
    2>&1 | tee cppcheck-report.txt

echo ""
echo "========================================="
echo "Анализ завершен!"
echo ""
echo "Отчеты сохранены в:"
echo "  XML:  cppcheck-report.xml"
echo "  TXT:  cppcheck-report.txt"
echo ""
echo "Просмотреть отчет:"
echo "  cat cppcheck-report.txt"
echo ""
echo "Для HTML отчета установите cppcheck-gui:"
echo "  sudo apt install cppcheck-gui"
echo "  cppcheck-gui cppcheck-report.xml"
echo "========================================="
