#!/usr/bin/env python3
"""Draws the black-and-orange 2600 front panel once and cuts it into three Rack rows.

Coordinates are pixels of a 2000 x 1470 front view of the hardware, measured
from that reference and redrawn as original artwork (tools/orange_layout.py).
The three rows are contiguous windows onto one drawing, so the stacked modules
read as a single panel. No reference image is packaged with the plugin.
"""
import math
from pathlib import Path
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen

root = Path(__file__).resolve().parents[1]
FONTS = Path.home()/'Library/Fonts'
ROW = 380; METER = 21  # Rack's CPU meter covers the bottom 21 px of every module.
S = 1.47               # vertical scale (px per reference unit); also sizes text, jacks and strokes
W = 1680               # 112 HP
# Each row's bottom METER px carry only signal lines, never controls or text.
PREFIX = 'CE2600Orange'
SUFFIX = 'Orange'
PANEL = '#141618'
INK = '#f6e7d7'
SLOT = '#050607'
RED = '#ffb15c'

ACCENT = '#ff8238'

# Units are pixels of the 2000 x 1470 front view. Each Rack row is a separate linear
# window: the dense top band is compressed, the lower bands keep near true scale.
XS = .84
ORANGE_ROWS = [(0, 586, 0, 359), (586, 1003, 382, 739), (1003, 1470, 762, 1144)]
def X(u): return u*XS
def Y(v):
    for v0, v1, y0, y1 in ORANGE_ROWS:
        if v <= v1 or v1 == 1470: return y0+(v-v0)*(y1-y0)/(v1-v0)
def n(v): return f'{v:.2f}'.rstrip('0').rstrip('.')

items = []  # (top px, bottom px, svg, solid) in whole-panel coordinates
def add(svg, y0, y1, solid=True): items.append((y0, y1, svg, solid))

class Face:
    def __init__(self, file):
        f = TTFont(file); self.glyphs = f.getGlyphSet(); self.cmap = f.getBestCmap(); self.upm = f['head'].unitsPerEm
bold = Face(FONTS/'Inter-Bold.otf'); italic = Face(FONTS/'Inter-BoldItalic.otf')
cond = Face(FONTS/'ArchivoNarrow-Bold.otf')

def text(s, x, y, size=5.0, anchor='middle', fill=INK, track=0.0, face=bold):
    """Baseline at (x, y); size is the em size in vertical panel units. Returns width in horizontal units."""
    px = size*S; scale = px/face.upm; names = [face.cmap[ord(c)] for c in s]
    width = sum(face.glyphs[g].width for g in names)*scale + track*S*(len(s)-1)
    cx = X(x) - {'middle': width/2, 'end': width}.get(anchor, 0); cy = Y(y)
    pen = SVGPathPen(face.glyphs, ntos=n)
    for g in names:
        face.glyphs[g].draw(TransformPen(pen, (scale, 0, 0, -scale, cx, cy))); cx += face.glyphs[g].width*scale + track*S
    if pen.getCommands(): add(f'<path d="{pen.getCommands()}" fill="{fill}"/>', cy-px*.75, cy+px*.25)
    return width/XS

def lines(s, x, y, size=5.0, lead=None, **k):
    lead = lead or size*1.22
    for j, row in enumerate(s.split('|')): text(row, x, y+j*lead, size, **k)

def path_px(pts, w=.75, color=INK, close=False, fill='none'):
    d = 'M' + ' L'.join(f'{n(a)} {n(b)}' for a, b in pts) + (' Z' if close else '')
    ys = [b for _, b in pts]
    add(f'<path d="{d}" fill="{fill}" stroke="{color}" stroke-width="{n(w*S)}" stroke-linejoin="round" stroke-linecap="round"/>', min(ys)-2, max(ys)+2, solid=False)
def path(pts, **k):
    k.setdefault("color", ACCENT)
    path_px([(X(a), Y(b)) for a, b in pts], **k)

def wire(*pts, end=True, start=False):
    """Signal line with arrowheads, computed in pixels so arrows keep their shape."""
    pts = [(X(a), Y(b)) for a, b in pts]; size = 4.0*S
    def head(tip, frm):
        (tx, ty), (fx, fy) = tip, frm; L = math.hypot(tx-fx, ty-fy); ux, uy = (tx-fx)/L, (ty-fy)/L
        path_px([tip, (tx-ux*size+uy*size*.4, ty-uy*size-ux*size*.4), (tx-ux*size-uy*size*.4, ty-uy*size+ux*size*.4)], w=.2, close=True, fill=ACCENT, color=ACCENT)
        return (tx-ux*size*.7, ty-uy*size*.7)
    if end: tip = head(pts[-1], pts[-2]); pts[-1] = tip
    if start: head(pts[0], pts[1])
    path_px(pts, color=ACCENT)

def dot(x, y, r=1.4): add(f'<circle cx="{n(X(x))}" cy="{n(Y(y))}" r="{n(r*S)}" fill="{INK}"/>', Y(y)-2, Y(y)+2, solid=False)
def circle(x, y, r, fill=PANEL, stroke=INK, w=.9):
    add(f'<circle cx="{n(X(x))}" cy="{n(Y(y))}" r="{n(r*S)}" fill="{fill}" stroke="{stroke}" stroke-width="{n(w*S)}"/>', Y(y)-r*S-1, Y(y)+r*S+1)

def rect(x0, y0, x1, y1, w=1.0, fill=PANEL, stroke=INK, r=0):
    add(f'<rect x="{n(X(x0))}" y="{n(Y(y0))}" width="{n(X(x1)-X(x0))}" height="{n(Y(y1)-Y(y0))}" rx="{n(r)}" fill="{fill}" stroke="{stroke}" stroke-width="{n(w*S)}"/>', Y(y0)-1, Y(y1)+1)

def block(x0, y0, x1, y1, title, size=6.6):
    rect(x0, y0, x1, y1, fill="#321d15", stroke=ACCENT, r=3)
    rows = title.split('|'); lead = size*1.05
    for j, row in enumerate(rows): text(row, (x0+x1)/2, (y0+y1)/2-(len(rows)-1)*lead/2+j*lead+size*.36, size)

SLOT_PX = 7
def vslot(x, y0, y1): add(f'<rect x="{n(X(x)-SLOT_PX/2)}" y="{n(Y(y0))}" width="{SLOT_PX}" height="{n(Y(y1)-Y(y0))}" rx="{SLOT_PX/2}" fill="{SLOT}"/>', Y(y0), Y(y1))
def hslot(x0, x1, y): add(f'<rect x="{n(X(x0))}" y="{n(Y(y)-SLOT_PX/2)}" width="{n(X(x1)-X(x0))}" height="{SLOT_PX}" rx="{SLOT_PX/2}" fill="{SLOT}"/>', Y(y)-SLOT_PX/2, Y(y)+SLOT_PX/2)
def ticks(x, y0, y1, side=1, count=5):
    for k in range(count):
        y = y0+6+k*(y1-y0-9)/(count-1); path([(x+side*11, y), (x+side*17, y)], w=.7)

def bracket(x0, x1, y, label, size=4.4):
    w = text(label, (x0+x1)/2, y+size*.36, size, track=.5)
    path([(x0, y+8), (x0, y), ((x0+x1)/2-w/2-2, y)], w=.6); path([((x0+x1)/2+w/2+2, y), (x1, y), (x1, y+8)], w=.6)

def wave(kind, x, y, w=13, h=4.5, lw=.7):
    """Waveform symbol centred on (x, y); w is in horizontal units."""
    l, r, t, b = x-w/2, x+w/2, y-h/2, y+h/2
    if kind == 'saw': pts = [(l, b)] + [p for k in range(3) for p in [(l+w*(k+1)/3, t), (l+w*(k+1)/3, b)]]
    elif kind == 'square': pts = [(l, b)] + [p for k in range(3) for p in [(l+w*k/3, t), (l+w*(k+.5)/3, t), (l+w*(k+.5)/3, b), (l+w*(k+1)/3, b)]]
    elif kind == 'pulse': pts = [(l, b)] + [p for k in range(4) for p in [(l+w*k/4, t), (l+w*(k+.3)/4, t), (l+w*(k+.3)/4, b), (l+w*(k+1)/4, b)]]
    elif kind == 'tri': pts = [(l+w*k/6, b if k % 2 == 0 else t) for k in range(7)]
    elif kind == 'sine': pts = [(l+w*k/40, y-h/2*math.sin(k/40*4*math.pi)) for k in range(41)]
    elif kind == 'clock': pts = [p for k in range(6) for p in [(l+w*k/6, b), (l+w*k/6, t), (l+w*k/6, b), (l+w*(k+1)/6, b)]]
    elif kind == 'gate': pts = [(l, b), (l+w*.3, b), (l+w*.3, t), (l+w*.7, t), (l+w*.7, b), (r, b)]
    elif kind == 'trig': pts = [(l, b), (x, b), (x, t), (x, b), (r, b)]
    elif kind == 'adsr': pts = [(l, b), (l+w*.25, t), (l+w*.45, y), (l+w*.75, y), (r, b)]
    elif kind == 'ar': pts = [(l, b), (l+w*.3, t), (l+w*.7, t), (r, b)]
    elif kind == 'lag': pts = [(l, b), (l+w*.25, b)] + [(l+w*.25+w*.75*k/12, b-(b-t)*(1-math.exp(-k/3.5))) for k in range(13)]
    elif kind == 'attack': pts = [(l, b), (l+w*.45, t), (r, t)]
    elif kind == 'release': pts = [(l, t), (l+w*.55, t), (r, b)]
    path(pts, w=lw)

def normal(x, label='', icon=None, y0=542, y1=571):
    """Boxed normalled source printed under a jack."""
    rect(x-15, y0, x+15, y1, w=.8); wire((x, y0), (x, 528))
    rows = [r for r in label.split('|') if r]; n_rows = len(rows)+(1 if icon else 0)
    top = (y0+y1)/2-(n_rows-1)*3.4
    for j, row in enumerate(rows): text(row, x, top+j*6.8+1.6, 4.3, track=.1 if ' ' in row else 0)
    if icon: wave(icon, x, top+len(rows)*6.8, w=17, h=4.5, lw=.6)

# Widgets ------------------------------------------------------------------
owners = {k: {} for k in ['param', 'input', 'output']}; layouts = [[] for _ in range(3)]
HALF = {'jack': 13, 'big': 22, 'knob': 20, 'sw': 13, 'swi': 13, 'sw3': 16, 'sw3i': 16, 'button': 11, 'rocker': 19, 'light': 5, 'h': 9}
def place(kind, id, x, y, style='jack', length=0, cap=0):
    """length is the slider travel in pixels."""
    px, py = X(x), Y(y); half = length/2 if style in ('v', 'vi') else HALF.get(style, 11)
    row = int(py//ROW); ry = py-row*ROW
    assert half <= ry <= ROW-METER-half, f'{id} at {x},{y} crosses a row seam or the CPU meter strip'
    if kind != 'light': assert id not in owners[kind], (kind, id); owners[kind][id] = row
    pos = f'Vec({n(px)}, {n(ry)})'
    if kind == 'light':
        light = 'MediumLight<OrangeLight2600>'
        layouts[row].append(f'addChild(createLightCentered<{light}>({pos}, module, CE2600::{id}));'); return
    if kind == 'param':
        L = round(length)
        assert style not in ('v', 'vi', 'h') or 1 <= cap <= 5, f'{id}: slider needs a cap colour 1-5'
        cls = {'v': f'VSlider2600<{L}, false, {cap}>', 'vi': f'VSlider2600<{L}, true, {cap}>', 'h': f'HSlider2600<{L}, {cap}>',
               'sw': 'SwitchOrange2600', 'swi': 'SwitchOrangeInverted2600', 'sw3': 'RangeOrange2600', 'sw3i': 'RangeOrangeInverted2600',
               'knob': 'KnobOrange2600', 'button': 'ButtonOrange2600', 'rocker': 'RockerOrange2600'}[style]
        layouts[row].append(f'addParam(createParamCentered<{cls}>({pos}, module, CE2600::{id}));')
    else:
        cls = 'BigJackOrange2600' if style == 'big' else 'JackOrange2600'
        method, helper = ('addInput', 'createInputCentered') if kind == 'input' else ('addOutput', 'createOutputCentered')
        layouts[row].append(f'{method}({helper}<{cls}>({pos}, module, CE2600::{id}));')

def jack_in(id, x, y): place('input', id, x, y)
def jack_out(id, x, y, style='jack'): place('output', id, x, y, style)
def vslider(id, x, y0=368, y1=470, inverted=False, tick=1):
    vslot(x, y0, y1)
    if tick: ticks(x, y0, y1, tick)
    place('param', id, x, (y0+y1)/2, 'vi' if inverted else 'v', Y(y1)-Y(y0))
def hslider(id, x0, x1, y):
    hslot(x0, x1, y); place('param', id, (x0+x1)/2, y, 'h', X(x1)-X(x0))
def switch(id, x, y, style='sw'): place('param', id, x, y, style)

from orange_layout import draw
draw(globals())
# Rows, layouts and ownership ---------------------------------------------------------
# Controls and print stay clear of row seams and of the CPU meter strip at the bottom of each row.
for y0, y1, svg, solid in items:
    if solid:
        for k in range(1, 4):
            assert not (y0 < k*ROW and y1 > k*ROW-METER), f'printed item at {y0:.0f}-{y1:.0f} px crosses row {k} seam or meter strip: {svg[:70]}'
for row, name in enumerate(['Top', 'Middle', 'Bottom']):
    body = [svg for y0, y1, svg, _ in items if y1 >= row*ROW and y0 <= (row+1)*ROW]
    (root/f'res/{PREFIX}{name}.svg').write_text('\n'.join([f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{ROW}" viewBox="0 0 {W} {ROW}">',
        f'<g transform="translate(0 {-row*ROW})">'] + body + ['</g>', '</svg>']) + '\n')
    (root/f'src/layout{SUFFIX}{name}.inc').write_text('\n'.join(layouts[row])+'\n')
out = ['#pragma once', f'inline int parameterPanel{SUFFIX}(int id) {{ switch(id) {{']
out += [f'case CE2600::{id}: return {row};' for id, row in owners['param'].items()]
out += ['default: return 0; } }', f'inline int inputPanel{SUFFIX}(int id) {{ switch(id) {{']
out += [f'case CE2600::{id}: return {row};' for id, row in owners['input'].items()]
out += ['default: return 0; } }']
(root/f'src/ownership{SUFFIX}.hpp').write_text('\n'.join(out)+'\n')

# Control artwork ---------------------------------------------------------------------
from orange_layout import controls
controls(globals())
print('Orange panels generated:', {k: len(v) for k, v in owners.items()})
