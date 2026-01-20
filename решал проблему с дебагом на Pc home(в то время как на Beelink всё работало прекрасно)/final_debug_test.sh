#!/bin/bash

echo "=== Финальная диагностика отладки STM32 ==="
echo

echo "1. Тестирование системного GDB с ARM файлом:"
/usr/bin/gdb -batch -ex "file build/Debug/Piano_H7.elf" -ex "show architecture" -ex "quit"
echo

echo "2. Запуск ST-Link сервера в фоновом режиме для тестирования:"
pkill -f "ST-LINK_gdbserver" 2>/dev/null || true
sleep 1

/home/sche/st/stm32cubeclt_1.19.0/STLink-gdb-server/bin/ST-LINK_gdbserver -cp /home/sche/st/stm32cubeclt_1.19.0/STM32CubeProgrammer/bin/ -s -p 3333 &
SERVER_PID=$!
sleep 3

echo "3. Проверка работы ST-Link сервера:"
if netstat -tln | grep -q ":3333"; then
    echo "✅ ST-Link сервер запущен на порту 3333"
else
    echo "❌ ST-Link сервер не запустился"
    echo "Последние строки вывода сервера:"
    tail -n 10 /tmp/stlink.log 2>/dev/null || echo "Лог файл недоступен"
fi

echo
echo "4. Тестирование подключения GDB к ST-Link серверу:"
timeout 10s /usr/bin/gdb -batch -ex "target remote localhost:3333" -ex "info registers" -ex "quit" 2>&1 || echo "GDB подключение завершено"

echo
echo "5. Остановка ST-Link сервера:"
kill $SERVER_PID 2>/dev/null || true
echo

echo "=== Результаты диагностики ==="
echo "Если все тесты прошли успешно, отладка должна работать в VSCode."
echo "Если есть проблемы, проверьте:"
echo "- Подключение микроконтроллера"
echo "- Настройки ST-Link сервера"
echo "- Права доступа к USB устройствам"