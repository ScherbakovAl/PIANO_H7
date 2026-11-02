#!/usr/bin/env python3
"""Макеты экранов для обзор_экранов.html — div-ы, собранные из display_piano.eez-project.

    python3 eez_proj/макеты.py

Раскладка, шрифты и цвета берутся из проекта EEZ. Сцены ниже задают то, что на устройстве
выставляет gui.cpp: тексты, видимость, отмеченные вкладки, значения, сетку плат, шкалу порогов,
панель подтверждения. Каждый макет в HTML стоит между <!-- макет: имя --> и <!-- /макет -->;
скрипт заменяет только эти места. Графики рисует <script> в конце HTML по id из сцены.
"""
import html
import json
import os
import re

here = os.path.dirname(os.path.abspath(__file__))
project = json.load(open(os.path.join(here, 'display_piano.eez-project')))
pages = {pg['name']: pg['components'][0] for pg in project['userPages']}

LINE = {10: 11, 12: 15, 14: 17, 18: 21, 24: 27}  # высота строки шрифтов ui_font_ru_N
TEXT = '#fafafa'


def styles(w, state):
    """свойства MAIN: DEFAULT, поверх — DISABLED или CHECKED, если сцена так говорит"""
    d = w.get('localStyles', {}).get('definition', {})
    out = dict(d.get('MAIN', {}).get('DEFAULT', {}))
    if state:
        out.update(d.get('MAIN', {}).get(state, {}))
    return out, d


def rgba(c, opa):
    c = c.lstrip('#')
    return 'rgba(%d,%d,%d,%.2f)' % (int(c[0:2], 16), int(c[2:4], 16), int(c[4:6], 16), opa / 255)


def place(w, ov):
    """координаты по align, как в LVGL"""
    x = ov.get('left', w['left'])
    y = ov.get('top', w['top'])
    al = styles(w, None)[0].get('align', 'TOP_LEFT')
    if al == 'CENTER':
        return ['left:50%', 'top:50%', 'transform:translate(calc(-50%% + %dpx),calc(-50%% + %dpx))' % (x, y)]
    if al == 'LEFT_MID':
        return ['left:%dpx' % x, 'top:50%', 'transform:translateY(calc(-50%% + %dpx))' % y]
    if al == 'RIGHT_MID':
        return ['right:%dpx' % -x, 'top:50%', 'transform:translateY(calc(-50%% + %dpx))' % y]
    if al == 'BOTTOM_LEFT':
        return ['left:%dpx' % x, 'bottom:%dpx' % -y]
    assert al == 'TOP_LEFT', al
    return ['left:%dpx' % x, 'top:%dpx' % y]


def size(w, ov):
    out = []
    for k in ('width', 'height'):
        v = ov.get(k, w[k])
        if w[k + 'Unit'] == 'px' or k in ov:
            out.append('%s:%dpx' % (k, v))
    return out


def box(s, kind):
    out = []
    if s.get('bg_opa', 255) and 'bg_color' in s and (kind != 'label' or 'bg_opa' in s):
        out.append('background:%s' % s['bg_color'])
    if kind in ('container', 'button', 'chart', 'roller') or 'radius' in s:
        out.append('border-radius:%dpx' % s.get('radius', 0))
    if s.get('border_width'):
        out.append('border:%dpx solid %s' % (s['border_width'], rgba(s['border_color'], s.get('border_opa', 255))))
    return out


def text_css(s):
    out = []
    if 'text_color' in s:
        out.append('color:%s' % s['text_color'])
    if 'text_font' in s:
        n = int(s['text_font'].split('_')[1])
        out += ['font-size:%dpx' % n, 'line-height:%dpx' % LINE[n]]
    if 'text_align' in s:
        out.append('text-align:%s' % s['text_align'].lower())
    return out


def label_font(s):
    n = int(s.get('text_font', 'ru_14').split('_')[1])
    return ['font-size:%dpx' % n, 'line-height:%dpx' % LINE[n]]


def div(cls, css, inner=''):
    return '<div class="%s" style="%s">%s</div>' % (cls, ';'.join(css), inner)


def widget(w, sc):
    ov = sc.get(w.get('identifier'), {})
    hidden = ov.get('hidden', w.get('hiddenFlag'))
    if hidden:
        return ''
    t = w['type'][4:-6].lower()
    state = 'DISABLED' if ov.get('disabled') else 'CHECKED' if ov.get('checked', w.get('checkedState')) else None
    s, d = styles(w, state)
    if 'color' in ov:
        s['text_color'] = ov['color']  # set_color() в gui.cpp
    css = place(w, ov) + size(w, ov)
    inner = ''.join(widget(c, sc) for c in w.get('children', []))
    inner += ov.get('extra', '')
    if t == 'label':
        text = html.escape(ov.get('text', w['text']))
        cls = 'w label'
        if w.get('longMode') == 'CLIP' and w['heightUnit'] == 'px':
            cls += ' clip'
        elif w['widthUnit'] == 'px':
            cls += ' wrap'
        pad = [s.get('pad_top', 0), s.get('pad_right', 0), s.get('pad_bottom', 0), s.get('pad_left', 0)]
        css += box(s, 'label') + [c for c in text_css(s) if not c.startswith(('font-size', 'line-height'))] + label_font(s)
        if any(pad):
            css.append('padding:%dpx %dpx %dpx %dpx' % tuple(pad))
        return div(cls, css + ['overflow:hidden'], text)
    if t in ('container', 'button'):
        return div('w ' + t, css + box(s, t) + text_css(s), inner)
    if t == 'chart':
        pl, pt = s.get('pad_left', 0), s.get('pad_top', 0)
        wd = ov.get('width', w['width']) - pl - s.get('pad_right', 0)
        ht = ov.get('height', w['height']) - pt - s.get('pad_bottom', 0)
        svg = '<svg class="w" id="%s" style="left:%dpx;top:%dpx" width="%d" height="%d" data-grid="%d"></svg>' % (
            ov['svg'], pl, pt, wd, ht, w['divLineCountH'])
        return div('w chart', css + box(s, t) + ['overflow:hidden'], svg)
    if t in ('bar', 'slider'):
        ind = d.get('INDICATOR', {}).get('DEFAULT', {}).get('bg_color', '#0dcaf3')
        knob = d.get('KNOB', {}).get('DEFAULT', {}).get('bg_color', ind)
        h = w['height']
        pct = round(100 * (ov.get('value', w['value']) - w['min']) / (w['max'] - w['min']))
        inner = '<i class="w" style="left:0;top:0;bottom:0;width:%d%%;background:%s;border-radius:%dpx"></i>' % (pct, ind, h // 2)
        if t == 'slider':
            k = h + 10
            inner += '<i class="w" style="left:calc(%d%% - %dpx);top:-5px;width:%dpx;height:%dpx;border-radius:50%%;background:%s"></i>' % (
                pct, k // 2, k, k, knob)
        return div('w ' + t, css + ['background:%s' % s.get('bg_color', '#3a3d42'), 'border-radius:%dpx' % (h // 2)], inner)
    if t == 'roller':
        opts = w['options'].split('\n')
        sel = ov.get('selected', w['selected'])
        sd = d.get('SELECTED', {}).get('DEFAULT', {})
        row = 35  # шаг строк роллера LVGL при шрифте 14
        mid = (w['height'] - row) // 2
        inner = '<div class="w" style="left:0;right:0;top:%dpx;height:%dpx;background:%s"></div>' % (mid, row, sd.get('bg_color'))
        for i, o in enumerate(opts):
            y = mid + (i - sel) * row
            if -row < y < w['height']:
                c = ';color:%s' % sd['text_color'] if i == sel and 'text_color' in sd else ''
                inner += '<div class="w" style="left:0;right:0;top:%dpx;height:%dpx;line-height:%dpx;text-align:center%s">%s</div>' % (
                    y, row, row, c, html.escape(o))
        return div('w roller', css + box(s, t) + text_css(s) + ['overflow:hidden'], inner)
    raise ValueError(t)


def screen(name, sc):
    s = pages[name]
    inner = ''.join(widget(c, sc) for c in s['children']) + sc.get('overlay', '')
    return '<div class="bezel"><div class="dev">%s</div></div>' % inner


# --------------------------------------------------------------- то, что создаёт gui.cpp

def rect(x, y, w, h, color, r=0):
    return div('w container', ['left:%dpx' % x, 'top:%dpx' % y, 'width:%dpx' % w, 'height:%dpx' % h,
                               'background:%s' % color, 'border-radius:%dpx' % r])


def chip_grid(states):
    """chip_grid_create: ячейки 17×17 с шагом 19, ряды ON 1–13 и OFF 14–26"""
    color = {'piano': '#2fc25b', 'boot1': '#eeab00', 'boot2': '#a98bf0', 'none': '#4a4e55'}
    out = ''
    for n in range(1, 27):
        row = 0 if n < 14 else 1
        col = n - 14 if row else n - 1
        out += rect(col * 19, row * 24, 17, 17, color[states.get(n, 'piano')], 4)
    return out


def scale(r, g, live, on):
    """calib_scale_create и scale_mark: 0..4095 на 280 px, R и G по разные стороны шкалы"""
    def x(v):
        return max(0, min(v, 4095)) * 277 // 4095

    def lbl(v, name, color, top):
        return div('w label wrap', ['left:%dpx' % max(0, min(x(v) - 28, 280 - 56)), 'top:%dpx' % top, 'width:56px',
                                    'color:%s' % color, 'font-size:12px', 'line-height:15px', 'text-align:center',
                                    'overflow:hidden'], '%s %d' % (name, v))
    out = rect(0, 21, 280, 6, '#3a3d42', 3)
    out += rect(x(r), 16, 3, 16, '#e8692f') + rect(x(g), 16, 3, 16, '#2fc25b') + rect(x(live), 16, 3, 16, '#0dcaf3')
    out += lbl(r, 'R', '#e8692f', 0 if on else 33) + lbl(g, 'G', '#2fc25b', 33 if on else 0)
    return out


def confirm(title, text, ok, danger):
    """confirm(): затемнение и карточка на lv_layer_top()"""
    def btn(x, t, bg, color):
        return div('w button', ['left:%dpx' % x, 'top:132px', 'width:120px', 'height:48px', 'background:%s' % bg,
                                'border-radius:8px'],
                   div('w label', ['left:50%', 'top:50%', 'transform:translate(-50%,-50%)', 'color:%s' % color,
                                   'font-size:14px', 'line-height:17px', 'overflow:hidden'], html.escape(t)))
    card = div('w label wrap', ['left:16px', 'top:16px', 'width:248px', 'font-size:14px', 'line-height:17px'], html.escape(title))
    card += div('w label wrap', ['left:16px', 'top:46px', 'width:248px', 'color:#8b9098', 'font-size:14px',
                                 'line-height:17px'], html.escape(text))
    card += btn(16, 'Отмена', '#343638', TEXT)
    card += btn(144, ok, '#7a1f22' if danger else '#343638', '#ffffff' if danger else '#eeab00')
    return div('w container', ['left:0', 'top:0', 'width:320px', 'height:480px', 'background:rgba(0,0,0,0.60)'],
               div('w container', ['left:20px', 'top:140px', 'width:280px', 'height:196px', 'background:#24272b',
                                   'border-radius:10px'], card))


LOG = ('0        *+*+*+*   piano   *+*+*+*\n1        calib H7 >> G4\n2        sync OK\n'
       '3         1 2 3 4 5 6 7 8 9 10 11 12 13 14 ... = 26\n         pianos\n')
STATUS = {'main_status_chips': {'text': 'G4: 26', 'color': '#2fc25b'}, 'main_status_usb': {'color': '#2fc25b'},
          'main_curve_name': {'text': 'lin speed'}, 'main_curve_modes': {'text': 'OFF lin · LIFT lin'}}
# главный: без строки «последняя нота» плитка и график длиннее на её место (main_last_note_layout)
tile, chart, note = pages['d_main']['children'][1], pages['d_main']['children'][1]['children'][3], pages['d_main']['children'][2]
grow = note['top'] + note['height'] - tile['top'] - tile['height']

SCENES = {
    'main': ('d_main', dict(STATUS, **{
        'main_curve_tile': {'height': tile['height'] + grow},
        'main_curve_chart': {'height': chart['height'] + grow, 'svg': 'mc-a'},
        'main_last_note': {'hidden': True},
    })),
    'main_note': ('d_main', dict(STATUS, **{
        'main_curve_chart': {'svg': 'mc-b'},
        'main_status_calib': {'hidden': False},
        'main_status_warn': {'hidden': False}, 'main_status_warn_label': {'text': '▲ 2'},
        'main_last_note_label': {'text': 'A4  ·  vel 87  ·  3.2 мс'},
        'main_last_note_bar': {'value': 87},
    })),
    'curve': ('d_curve', {
        'curve_save': {'hidden': False},
        'curve_chart': {'svg': 'cc'},
        'curve_axis_min': {'text': '0.2 мс'}, 'curve_axis_max': {'text': '30.0 мс'}, 'curve_info': {'text': '2 мс → 12'},
        'curve_roller': {'selected': 11},
        'curve_v_fast': {'text': '0.39 мс'}, 'curve_s_fast': {'value': 390},
        'curve_v_slow': {'text': '14.0 мс'}, 'curve_s_slow': {'value': 14000},
        'curve_v_x0': {'text': '0.30 мс'}, 'curve_s_x0': {'value': 300},
    }),
    'curve_off': ('d_curve', {
        'curve_tab_on': {'checked': False}, 'curve_tab_off': {'checked': True},
        'curve_chart': {'svg': 'cc-off'},
        'curve_axis_min': {'text': '0.2 мс'}, 'curve_axis_max': {'text': '18.0 мс'}, 'curve_info': {'text': 't, лог. шкала'},
        'curve_roller': {'hidden': True}, 'curve_sliders': {'hidden': True},
        'curve_mode_info': {'hidden': False, 'text': '127 при\n0.4 мс\n\n1 при\n12.0 мс'},
        'curve_mode_panel': {'hidden': False},
    }),
    'calib': ('d_calib', {
        'calib_save_label': {'text': 'Сохр. 3'},
        'calib_chart': {'svg': 'cal'},
        'calib_key': {'text': 'Клавиша 23'}, 'calib_key_info': {'text': 'G2 · чип 4 · комп. 1'},
        'calib_live': {'text': '1834'}, 'calib_neighbors': {'text': '← 1790 · 1851 →'},
        'calib_margin': {'text': 'G − R  521'},
        'calib_scale': {'extra': scale(1580, 2101, 1834, True)},
        'calib_g_change': {'text': '2101 → 1834'}, 'calib_r_change': {'text': '1580 → 1834'},
    }),
    'service': ('d_service', {
        'service_chips_summary': {'text': '25 в piano · 1 в boot 1'},
        'service_chip_grid': {'extra': chip_grid({9: 'boot1'})},
        'service_warn': {'hidden': False, 'text': '▲ 2'},
    }),
    'flash': ('d_flash', {
        'flash_state': {'text': '26 в boot 1'},
        'flash_step1': {'disabled': True},
        'flash_log': {'text': LOG + '4        reset to bootloader\n5         1 2 3 4 5 6 7 8 9 10 11 12 13 14 ... = 26\n'
                              '         bootloaders\n6        *+*+*+*   bootloader   *+*+*+*'},
    }),
    'log_confirm': ('d_log', {
        'log_counters': {'text': 'USART noise 2 · MIDI lost 0 · G4 ring 0\nLCD DMA 0 · Touch I2C 0'},
        'log_text': {'text': LOG + '4        calib G4 >> H7\n5        calib saved\n'
                             '6        USART noise 1 : 0-0-255-0\n7        USART noise 2 : 0-0-255-0'},
        'overlay': confirm('Перейти в USB DFU?', 'H7 перезагрузится в DFU-загрузчик. Пианино замолчит.', 'Перейти', True),
    }),
}

path = os.path.join(here, 'обзор_экранов.html')
doc = open(path).read()
for name, (page, sc) in SCENES.items():
    pat = re.compile(r'(<!-- макет: %s -->).*?(<!-- /макет -->)' % name, re.S)
    assert len(pat.findall(doc)) == 1, name
    doc = pat.sub(lambda m: m.group(1) + screen(page, sc) + m.group(2), doc)
open(path, 'w').write(doc)
print('макетов:', len(SCENES))
