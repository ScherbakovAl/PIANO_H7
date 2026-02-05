#!/bin/bash

# Скрипт для проверки зависимостей CubeCLT GDB
# Использование: ./DEPENDENCY_CHECK_SCRIPT.sh

echo "=== Проверка зависимостей для CubeCLT GDB ==="
echo

# Функция для проверки библиотеки
check_library() {
    local lib_name="$1"
    local lib_path=$(ldconfig -p | grep "$lib_name")
    
    if [ -n "$lib_path" ]; then
        echo "✅ $lib_name найдена: $lib_path"
        return 0
    else
        echo "❌ $lib_name НЕ НАЙДЕНА"
        return 1
    fi
}

# Функция для проверки файла
check_file() {
    local file_path="$1"
    local file_name=$(basename "$file_path")
    
    if [ -f "$file_path" ]; then
        echo "✅ $file_name найден: $file_path"
        return 0
    else
        echo "❌ $file_name НЕ НАЙДЕН: $file_path"
        return 1
    fi
}

echo "1. Проверка 32-битных библиотек для CubeCLT GDB:"
echo "=================================================="

# Список необходимых библиотек для arm-none-eabi-gdb
required_libs=(
    "libncurses.so.5"
    "libtinfo.so.5"
    "libdl.so.2"
    "libstdc++.so.6"
    "libm.so.6"
    "libgcc_s.so.1"
    "libpthread.so.0"
    "libc.so.6"
)

missing_libs=0
for lib in "${required_libs[@]}"; do
    if ! check_library "$lib"; then
        missing_libs=$((missing_libs + 1))
    fi
done

echo
echo "2. Проверка инструментов CubeCLT:"
echo "================================="

# Проверка CubeCLT инструментов
cubeclt_path="/home/sche/st/stm32cubeclt_1.19.0"
tools_to_check=(
    "$cubeclt_path/GNU-tools-for-STM32/bin/arm-none-eabi-gdb"
    "$cubeclt_path/GNU-tools-for-STM32/bin/arm-none-eabi-objdump"
    "$cubeclt_path/GNU-tools-for-STM32/bin/arm-none-eabi-nm"
    "$cubeclt_path/STLink-gdb-server/bin/ST-LINK_gdbserver"
    "$cubeclt_path/STM32CubeProgrammer/bin/STM32_Programmer_CLI"
)

missing_tools=0
for tool in "${tools_to_check[@]}"; do
    if ! check_file "$tool"; then
        missing_tools=$((missing_tools + 1))
    fi
done

echo
echo "3. Проверка пользователя и групп:"
echo "==================================="

current_user=$(whoami)
echo "Текущий пользователь: $current_user"

if groups "$current_user" | grep -q "plugdev"; then
    echo "✅ Пользователь состоит в группе plugdev"
else
    echo "❌ Пользователь НЕ состоит в группе plugdev"
    echo "   Добавьте пользователя: sudo usermod -a -G plugdev $current_user"
fi

echo
echo "4. Проверка ST-Link устройства:"
echo "================================="

if lsusb | grep -q "0483:3748"; then
    echo "✅ ST-Link устройство обнаружено:"
    lsusb | grep "0483:3748"
else
    echo "❌ ST-Link устройство НЕ обнаружено"
fi

echo
echo "5. Проверка правил udev:"
echo "=========================="

if [ -f "/etc/udev/rules.d/49-stlinkv2.rules" ]; then
    echo "✅ Правила udev для ST-Link найдены"
    echo "Содержимое правил:"
    grep -E "ATTRS{idVendor}==\"0483\"" /etc/udev/rules.d/49-stlink*.rules
else
    echo "❌ Правила udev для ST-Link НЕ найдены"
fi

echo
echo "6. Тест системного GDB:"
echo "========================="

if command -v gdb >/dev/null 2>&1; then
    echo "✅ Системный GDB найден: $(which gdb)"
    echo "Версия:"
    gdb --version | head -1
else
    echo "❌ Системный GDB НЕ найден"
fi

echo
echo "7. Тест подключения к микроконтроллеру:"
echo "======================================="

if [ -f "$cubeclt_path/STM32CubeProgrammer/bin/STM32_Programmer_CLI" ]; then
    echo "Проверка подключения к микроконтроллеру (требуется ST-Link)..."
    timeout 10s "$cubeclt_path/STM32CubeProgrammer/bin/STM32_Programmer_CLI" -c port=SWD mode=UR 2>&1 | grep -E "(Device name|Device ID|Error)" || echo "Тест не удался или ST-Link не подключен"
else
    echo "❌ STM32_Programmer_CLI не найден"
fi

echo
echo "=== РЕЗУЛЬТАТЫ ДИАГНОСТИКИ ==="
echo "==============================="

if [ $missing_libs -eq 0 ] && [ $missing_tools -eq 0 ]; then
    echo "✅ Все зависимости найдены! CubeCLT должен работать."
    echo "Если GDB все еще не работает, попробуйте системный GDB."
else
    echo "❌ Обнаружены проблемы с зависимостями:"
    if [ $missing_libs -gt 0 ]; then
        echo "   - Отсутствует $missing_libs библиотек"
        echo "   - Попробуйте: sudo dnf install ncurses-compat-libs libtinfo.i686"
    fi
    if [ $missing_tools -gt 0 ]; then
        echo "   - Отсутствует $missing_tools инструментов CubeCLT"
        echo "   - Проверьте установку CubeCLT"
    fi
fi

echo
echo "РЕКОМЕНДАЦИЯ: Используйте системный GDB для лучшей совместимости"
echo "Настройка в launch.json: \"gdbPath\": \"/usr/bin/gdb\""