#!/bin/bash

echo "=== Диагностика системы отладки STM32 ==="
echo

echo "1. Проверка ST-Link устройства:"
lsusb | grep -i st
echo

echo "2. Проверка прав доступа к USB устройству:"
ls -la /dev/bus/usb/*/ | grep -i st || echo "ST-Link устройство не найдено в /dev/bus/usb/"
echo

echo "3. Проверка групп пользователя:"
groups
echo

echo "4. Проверка наличия ST-Link сервера:"
ls -la /home/sche/st/stm32cubeclt_1.19.0/STLink-gdb-server/bin/ST-LINK_gdbserver
echo

echo "5. Проверка наличия STM32CubeProgrammer:"
ls -la /home/sche/st/stm32cubeclt_1.19.0/STM32CubeProgrammer/bin/STM32_Programmer_CLI
echo

echo "6. Проверка наличия GDB:"
ls -la /home/sche/st/stm32cubeclt_1.19.0/GNU-tools-for-STM32/bin/arm-none-eabi-gdb
echo

echo "7. Проверка наличия SVD файла:"
ls -la /home/sche/st/stm32cubeclt_1.19.0/STMicroelectronics_CMSIS_SVD/STM32H723.svd
echo

echo "8. Проверка собранных файлов прошивки:"
find . -name "*.elf" -type f
echo

echo "9. Проверка правил udev для ST-Link:"
ls -la /etc/udev/rules.d/ | grep -i st
echo

echo "10. Тест подключения к микроконтроллеру через STM32CubeProgrammer:"
/home/sche/st/stm32cubeclt_1.19.0/STM32CubeProgrammer/bin/STM32_Programmer_CLI -c port=SWD mode=UR
echo

echo "=== Рекомендации по устранению проблем ==="
echo
echo "Если ST-Link сервер показывает 'Unknown MCU found on target':"
echo "1. Убедитесь, что микроконтроллер не находится в режиме сна"
echo "2. Проверьте подключение SWD проводов"
echo "3. Попробуйте сбросить микроконтроллер (硬件 сброс)"
echo "4. Используйте более низкую частоту SWD"
echo
echo "Если есть проблемы с правами доступа:"
echo "1. Добавьте пользователя в группу plugdev: sudo usermod -a -G plugdev \$USER"
echo "2. Перезайдите в систему"
echo "3. Переподключите ST-Link устройство"
echo
echo "Для получения подробной диагностики отладки:"
echo "1. В VSCode откройте launch.json"
echo "2. Убедитесь, что включен 'showDevDebugOutput': 'raw'"
echo "3. Запустите отладку и проверьте вывод в Debug Console"