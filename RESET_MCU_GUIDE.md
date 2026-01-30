# Команды для перезагрузки микроконтроллера STM32H7

## 🔄 Перезагрузка через STM32_Programmer_CLI

### Вариант 1: Программный сброс (Software Reset)
Это самый быстрый способ перезагрузки микроконтроллера:

```bash
STM32_Programmer_CLI --connect port=swd sn=470065000D0000544B52524E -rst
```

### Вариант 2: Аппаратный сброс (Hardware Reset)
Более "жесткая" перезагрузка с аппаратным сбросом:

```bash
STM32_Programmer_CLI --connect port=swd sn=470065000D0000544B52524E -hardRst
```

### Вариант 3: Сброс и запуск программы
Выполняет сброс и автоматически запускает программу:

```bash
STM32_Programmer_CLI --connect port=swd sn=470065000D0000544B52524E -rst --start
```

### Вариант 4: Полный цикл - подключение, сброс, запуск и отключение
```bash
STM32_Programmer_CLI --connect port=swd sn=470065000D0000544B52524E -hardRst -rst --start
```

---

## 📌 Дополнительная информация

### Параметры команды:
- `--connect port=swd` - подключение через SWD интерфейс
- `sn=470065000D0000544B52524E` - серийный номер вашего ST-Link программатора (из tasks.json)
- `-rst` - программный сброс
- `-hardRst` - аппаратный сброс
- `--start` - запуск программы после сброса

### Для подключения через DFU (USB):
```bash
STM32_Programmer_CLI --connect port=usb1 -rst
```

---

## ⚙️ Добавление в VSCode tasks.json

Чтобы добавить кнопку для перезагрузки в панель VSCode, добавьте в файл `.vscode/tasks.json` следующую задачу:

```json
{
    "type": "shell",
    "label": "Reset MCU",
    "command": "STM32_Programmer_CLI",
    "args": [
        "--connect",
        "port=swd",
        "sn=470065000D0000544B52524E",
        "-hardRst",
        "-rst",
        "--start"
    ],
    "options": {
        "cwd": "${workspaceFolder}",
        "statusbar": {
            "color": "#ff9800",
            "label": "🔄 RESET",
            "detail": "Reset microcontroller"
        }
    },
    "problemMatcher": []
}
```

После добавления этой задачи, в панели задач VSCode появится оранжевая кнопка "🔄 RESET" для быстрой перезагрузки микроконтроллера!

---

## 🛠️ Альтернативные методы (если не работает STM32_Programmer_CLI)

### Через OpenOCD:
```bash
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg -c "init; reset; exit"
```

### Через st-flash:
```bash
st-flash reset
```

---

## 💡 Рекомендуемый вариант
Для повседневного использования рекомендую **Вариант 3** (сброс и запуск):
```bash
STM32_Programmer_CLI --connect port=swd sn=470065000D0000544B52524E -rst --start
```

Эта команда быстрая, надежная и автоматически запускает программу после сброса.
