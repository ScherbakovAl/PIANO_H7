#!/bin/bash
# Скрипт для анализа проекта Piano_H7 с помощью Cppcheck (бесплатная альтернатива)

set -e

PROJECT_DIR="/home/sche/Documents/programming/Piano/PIANO_H7"

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
cppcheck \
    --enable=all \
    --inconclusive \
    --std=c++20 \
    --platform=unix32 \
    --suppress=missingIncludeSystem \
    --suppress=unmatchedSuppression \
    --inline-suppr \
    -I Core/Inc \
    -I Piano_H7 \
    -I Drivers/CMSIS/Include \
    -I Drivers/CMSIS/Device/ST/STM32H7xx/Include \
    -I Drivers/STM32H7xx_HAL_Driver/Inc \
    -i Drivers/ \
    -i lvgl/ \
    -i eez_proj/ \
    -i H7_dfu_bootloader/Drivers/ \
    --xml \
    --xml-version=2 \
    Piano_H7/ \
    2> cppcheck-report.xml

echo ""
echo "[2/2] Генерация отчетов..."

# Текстовый отчет
cppcheck \
    --enable=all \
    --inconclusive \
    --std=c++20 \
    --platform=unix32 \
    --suppress=missingIncludeSystem \
    --suppress=unmatchedSuppression \
    --inline-suppr \
    -I Core/Inc \
    -I Piano_H7 \
    -I Drivers/CMSIS/Include \
    -I Drivers/CMSIS/Device/ST/STM32H7xx/Include \
    -I Drivers/STM32H7xx_HAL_Driver/Inc \
    -i Drivers/ \
    -i lvgl/ \
    -i eez_proj/ \
    -i H7_dfu_bootloader/Drivers/ \
    Piano_H7/ \
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
