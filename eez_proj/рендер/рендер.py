#!/usr/bin/env python3
"""Рендеры экранов для README (eez_proj/рендеры/*.png) — LVGL на ПК из кода интерфейса прошивки.

    python3 eez_proj/рендер/рендер.py

Собирает хостовым gcc LVGL, ui/ (EEZ) и Piano_H7/gui.cpp как есть, вместо железа и piano_h7.cpp —
stub/ и сцены.cpp. Сцены (состояние прошивки на каждом экране) — в сцены.cpp. Объекты LVGL и ui/
кэшируются в build/рендер, пересобираются, только если исходник новее.
"""
import concurrent.futures
import glob
import os
import struct
import subprocess
import zlib

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.normpath(os.path.join(here, '..', '..'))
fw = os.path.join(root, 'Piano_H7')
build = os.path.join(root, 'build', 'рендер')
out = os.path.join(root, 'eez_proj', 'рендеры')

SCENES = ['1_main', '1b_main_note', '2_curve', '2b_curve_off', '3_calib', '4_service', '5_flash', '6_log_confirm']
FLAGS = ['-O1', '-w', '-DLV_CONF_INCLUDE_SIMPLE', '-I', os.path.join(here, 'stub'),
         '-I', os.path.join(fw, 'lvgl'), '-I', fw, '-I', os.path.join(fw, 'ui')]


def compile_c(src):
    obj = os.path.join(build, 'obj', os.path.relpath(src, fw).replace(os.sep, '_') + '.o')
    if not os.path.exists(obj) or os.path.getmtime(obj) < os.path.getmtime(src):
        subprocess.run(['gcc', '-c', src, '-o', obj] + FLAGS, check=True)
    return obj


def png(path, w, h, rgb):
    rows = b''.join(b'\0' + rgb[y * w * 3:(y + 1) * w * 3] for y in range(h))

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(rows, 9)) + chunk(b'IEND', b''))


os.makedirs(os.path.join(build, 'obj'), exist_ok=True)
srcs = glob.glob(os.path.join(fw, 'lvgl', 'src', '**', '*.c'), recursive=True) + glob.glob(os.path.join(fw, 'ui', '*.c'))
with concurrent.futures.ThreadPoolExecutor(os.cpu_count()) as ex:
    objs = list(ex.map(compile_c, srcs))
exe = os.path.join(build, 'сцены')
subprocess.run(['g++', '-std=gnu++17', os.path.join(fw, 'gui.cpp'), os.path.join(here, 'сцены.cpp')] + objs
               + FLAGS + ['-lm', '-o', exe], check=True)

for name in SCENES:
    ppm = os.path.join(build, name + '.ppm')
    subprocess.run([exe, name, ppm], check=True)
    data = open(ppm, 'rb').read()
    png(os.path.join(out, name + '.png'), 320, 480, data[len(b'P6\n320 480\n255\n'):])
print('рендеров:', len(SCENES))
