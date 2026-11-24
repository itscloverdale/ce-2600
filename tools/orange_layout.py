"""Orange panel: control placement and print measured from the manufacturer's 2000 x 1470 front view.

Every coordinate below is a pixel of that front view; make_three_panel.py maps it into the three
Rack rows. The artwork is redrawn here as original vector print (CE mark instead of the maker's
logo). Sizes passed as `em` are print heights in front-view pixels.
"""
import math

WHITE = '#f3f0eb'; ORANGE = '#ff662c'; LINE = '#f2632b'; TRACK = '#020202'; LED = '#fc9702'
BLUE, GREEN, RED, YELLOW, CLEAR = 1, 2, 3, 4, 5  # slider cap colours, index into OrangeCap*.svg + 1
CAPS = {BLUE: '#3fb0ff', GREEN: '#38f26a', RED: '#ff3b30', YELLOW: '#ffd23c', CLEAR: '#ffffff'}
TS = .80  # Rack pixels of print per front-view pixel


def draw(p):
    X, Y, S, add, n = p['X'], p['Y'], p['S'], p['add'], p['n']
    place = p['place']

    # Print ---------------------------------------------------------------------------
    def t(s, x, y, em=12, anchor='middle', fill=WHITE, face=None):
        return p['text'](s, x, y, em*TS/S, anchor=anchor, fill=fill, face=face or p['cond'])

    def tl(s, x, y, em=12, lead=None, **k):
        """Stacked lines; the leading is fixed in Rack pixels so compressed rows never overlap."""
        lead = (lead or em*1.02)*TS
        for j, row in enumerate(s.split('|')): t(row, x, inv(Y(y)+j*lead), em, **k)

    def px_path(pts, w=1.5, color=LINE, fill='none', close=False, solid=False):
        d = 'M'+' L'.join(f'{n(a)} {n(b)}' for a, b in pts)+(' Z' if close else '')
        ys = [b for _, b in pts]
        add(f'<path d="{d}" fill="{fill}" stroke="{color}" stroke-width="{n(w)}" stroke-linejoin="round" stroke-linecap="round"/>', min(ys)-1, max(ys)+1, solid)

    def ln(*pts, w=1.5, color=LINE): px_path([(X(a), Y(b)) for a, b in pts], w, color)

    def arrow(*pts, start=False, w=1.5):
        """Signal line; arrowheads are drawn in pixels so they keep their shape in every row."""
        q = [(X(a), Y(b)) for a, b in pts]; size = 6.5
        def head(tip, frm):
            (tx, ty), (fx, fy) = tip, frm; L = math.hypot(tx-fx, ty-fy) or 1; ux, uy = (tx-fx)/L, (ty-fy)/L
            px_path([tip, (tx-ux*size+uy*size*.42, ty-uy*size-ux*size*.42), (tx-ux*size-uy*size*.42, ty-uy*size+ux*size*.42)], .4, LINE, LINE, True)
            return (tx-ux*size*.8, ty-uy*size*.8)
        q[-1] = head(q[-1], q[-2])
        if start: q[0] = head(q[0], q[1])
        px_path(q, w)

    def dot(x, y, r=2.6): add(f'<circle cx="{n(X(x))}" cy="{n(Y(y))}" r="{r}" fill="{LINE}"/>', Y(y)-r, Y(y)+r, False)

    def rect(x0, y0, x1, y1, fill=ORANGE, stroke='none', w=0, r=0, solid=True):
        add(f'<rect x="{n(X(x0))}" y="{n(Y(y0))}" width="{n(X(x1)-X(x0))}" height="{n(Y(y1)-Y(y0))}" rx="{r}" fill="{fill}" stroke="{stroke}" stroke-width="{w}"/>', Y(y0), Y(y1), solid)

    def block(x0, y0, x1, y1, title, em=14, lead=None):
        """Filled orange section title."""
        rect(x0, y0, x1, y1)
        rows = title.split('|'); lead = (lead or em*1.0)/TS*TS
        mid = (Y(y0)+Y(y1))/2; cap = em*TS*.686
        for j, row in enumerate(rows):
            base = mid+cap/2+(j-(len(rows)-1)/2)*lead*TS
            t(row, (x0+x1)/2, inv(base), em)

    def inv(py):
        """Front-view y for a Rack y inside the same row (used to centre print in boxes)."""
        for v0, v1, y0, y1 in reversed(p['ORANGE_ROWS']):
            if py >= y0-10: return v0+(py-y0)*(v1-v0)/(y1-y0)
        raise ValueError(py)

    def icon(kind, x, y, w=15, h=6, color=WHITE, lw=1.3):
        """Waveform and envelope symbols, sized in Rack pixels around a front-view point."""
        cx, cy = X(x), Y(y); l, r, top, b = cx-w/2, cx+w/2, cy-h/2, cy+h/2
        if kind == 'saw': pts = [(l, b)]+[q for k in range(3) for q in [(l+w*(k+1)/3, top), (l+w*(k+1)/3, b)]]
        elif kind == 'pulse': pts = [(l, b)]+[q for k in range(3) for q in [(l+w*k/3, top), (l+w*(k+.45)/3, top), (l+w*(k+.45)/3, b), (l+w*(k+1)/3, b)]]
        elif kind == 'tri': pts = [(l+w*k/6, b if k % 2 == 0 else top) for k in range(7)]
        elif kind == 'sine': pts = [(l+w*k/40, cy-h/2*math.sin(k/40*4*math.pi)) for k in range(41)]
        elif kind == 'clock': pts = [q for k in range(5) for q in [(l+w*k/5, b), (l+w*k/5, top), (l+w*(k+.5)/5, top), (l+w*(k+.5)/5, b), (l+w*(k+1)/5, b)]]
        elif kind == 'comb': pts = [q for k in range(6) for q in [(l+w*k/6, b), (l+w*k/6, top), (l+w*k/6, b), (l+w*(k+1)/6, b)]]
        elif kind == 'gate': pts = [(l, b), (l+w*.3, b), (l+w*.3, top), (l+w*.7, top), (l+w*.7, b), (r, b)]
        elif kind == 'trig': pts = [(l, b), (cx, b), (cx, top), (cx, b), (r, b)]
        elif kind == 'adsr': pts = [(l, b), (l+w*.2, b), (l+w*.38, top), (l+w*.55, cy), (l+w*.78, cy), (l+w*.9, b), (r, b)]
        elif kind == 'ar': pts = [(l, b), (l+w*.18, b), (l+w*.35, top), (l+w*.65, top), (l+w*.82, b), (r, b)]
        elif kind == 'lag': pts = [(l, b), (l+w*.2, b)]+[(l+w*.2+w*.8*k/12, b-(b-top)*(1-math.exp(-k/3.2))) for k in range(13)]
        px_path(pts, lw, color, solid=True)

    def label_box(x, title, kind=None, y0=958, y1=995, half=19):
        """Normalled source printed in an orange box under a jack."""
        rect(x-half, y0, x+half, y1)
        rows = [r for r in title.split('|') if r]
        mid = (Y(y0)+Y(y1))/2; em = 11; cap = em*TS*.686; lead = 10.2
        if kind and not rows: icon(kind, x, inv(mid), 18, 8); return
        count = len(rows)+(1 if kind else 0); top = mid-(count-1)*lead/2
        for j, row in enumerate(rows): t(row, x, inv(top+j*lead+cap/2), em)
        if kind: icon(kind, x, inv(top+len(rows)*lead), 18, 6.5)

    def normal_mark(x, y):
        """Circle symbol for a normalled connection, with a short lead from the jack."""
        cx, cy = X(x), Y(y); r = 8
        add(f'<circle cx="{n(cx)}" cy="{n(cy)}" r="{r}" fill="none" stroke="{LINE}" stroke-width="1.4"/>', cy-r, cy+r)
        px_path([(cx-3.6, cy-4.5), (cx-3.6, cy+1.5), (cx+.2, cy+1.5)], 1.3, solid=True)
        px_path([(cx-1.2, cy-1.5), (cx+4.8, cy-1.5), (cx+4.8, cy+1)], 1.3, solid=True)
        px_path([(cx+1.4, cy+r), (cx+1.4, cy+3.6)], 1.3, solid=True)
        px_path([(cx+1.4, cy+2.2), (cx-.4, cy+5), (cx+3.2, cy+5)], .4, LINE, LINE, True, solid=True)

    def jack(id, x, y, out=False, style='jack'):
        cx, cy = X(x), Y(y); r = 15.5 if style == 'jack' else 25
        add(f'<circle cx="{n(cx)}" cy="{n(cy)}" r="{r}" fill="#0c0c0d"/>', cy-r, cy+r, False)
        place('output' if out else 'input', id, x, y, style)

    def norm_jack(id, x, title, kind=None, out=False, y=905):
        jack(id, x, y, out); ln((x, y+19), (x, 928)); normal_mark(x, 940); label_box(x, title, kind)

    def vslider(id, x, y0, y1, cap, ticks=1, inverted=False):
        """Long black slot with an LED cap; white scale dashes on the right."""
        a, b = Y(y0), Y(y1); w = 12
        add(f'<rect x="{n(X(x)-w/2)}" y="{n(a)}" width="{w}" height="{n(b-a)}" rx="{w/2}" fill="{TRACK}" stroke="#454545" stroke-width=".8"/>', a, b)
        if ticks:
            for k in range(5):
                ty = a+(b-a)*(.1+.2*k); px_path([(X(x)+15, ty), (X(x)+21, ty)], 1.4, '#ececec', solid=True)
        place('param', id, x, (y0+y1)/2, 'vi' if inverted else 'v', b-a, cap)

    def hslider(id, x0, x1, y, cap):
        a, b = X(x0), X(x1); h = 11; cy = Y(y)
        add(f'<rect x="{n(a)}" y="{n(cy-h/2)}" width="{n(b-a)}" height="{h}" rx="{h/2}" fill="{TRACK}" stroke="#454545" stroke-width=".8"/>', cy-h/2, cy+h/2)
        place('param', id, (x0+x1)/2, y, 'h', b-a, cap)

    def switch(id, x, y, style='sw'): place('param', id, x, y, style)

    def dummy(x, y, r=8.5):
        """Unpopulated hole in the panel print."""
        cx, cy = X(x), Y(y)
        add(f'<circle cx="{n(cx)}" cy="{n(cy)}" r="{r}" fill="#262627" stroke="#1a1a1b" stroke-width="1"/>', cy-r, cy+r, False)

    def bracket(x0, x1, y, title, em=11):
        w = t(title, (x0+x1)/2, y+4, em)
        gap = w/2+5
        ln((x0, y+9), (x0, y), ((x0+x1)/2-gap, y)); ln(((x0+x1)/2+gap, y), (x1, y), (x1, y+9))

    def screw(x, y):
        cx, cy = X(x), Y(y); r = 7.5
        add(f'<circle cx="{n(cx)}" cy="{n(cy)}" r="{r}" fill="#1d1d1e" stroke="#0d0d0d" stroke-width="1"/>'
            f'<path d="M{n(cx-4)} {n(cy)}H{n(cx+4)}M{n(cx)} {n(cy-4)}V{n(cy+4)}" stroke="#4b4b4c" stroke-width="1.6"/>', cy-r, cy+r, False)

    def logo(x, y, width):
        """Cloverdale Engineering wordmark (tools/cloverdale-engineering-logo.svg), centred on a front-view point, width in Rack pixels."""
        import re
        src = (p['root']/'tools/cloverdale-engineering-logo.svg').read_text()
        vx, vy, vw, vh = map(float, re.search(r'viewBox="([^"]+)"', src).group(1).split())
        tags = re.findall(r'<path\b[^>]*>', src)  # the hidden badge outline is skipped
        paths = [re.search(r'\bd="([^"]+)"', tag).group(1) for tag in tags if 'display="none"' not in tag]
        k = width/vw; ox, oy = X(x)-width/2-vx*k, Y(y)-vh*k/2-vy*k
        body = ''.join(f'<path d="{d}" fill="{WHITE}"/>' for d in paths)
        add(f'<g transform="translate({n(ox)} {n(oy)}) scale({k:.5f})">{body}</g>', Y(y)-vh*k/2, Y(y)+vh*k/2)

    # Panel, rack ears and screws -------------------------------------------------------
    def shade(v):
        k = min(1, max(0, (v-60)/1040)); return tuple(round(a+(b-a)*k) for a, b in zip((78, 78, 80), (37, 37, 38)))
    # Panel shading runs continuously down all three rows, drawn as bands for Rack's SVG renderer.
    stops = [(0, shade(0)), (380, shade(586)), (760, shade(1003)), (1140, shade(1470))]
    for y in range(0, 1140, 4):
        (a, ca), (b, cb) = next((q, r) for q, r in zip(stops, stops[1:]) if q[0] <= y < r[0])
        c = '#'+''.join(f'{round(u+(v-u)*(y+2-a)/(b-a)):02x}' for u, v in zip(ca, cb))
        add(f'<rect x="0" y="{y}" width="{p["W"]}" height="4.5" fill="{c}"/>', y, y+4, False)
    for x in (40, 1960):
        for y in (158, 556, 890, 1310):
            add(f'<ellipse cx="{n(X(x))}" cy="{n(Y(y))}" rx="16" ry="10" fill="#d4d4d1" stroke="#151515" stroke-width="1"/>', Y(y)-10, Y(y)+10)
    for x in (120, 560, 1000, 1440, 1880): screw(x, 30); screw(x, 1446)
    screw(115, 487); screw(1885, 487); screw(1885, 1010)  # no left screw beside KBD IN

    # Title and maker mark (CE mark in place of the original logo).
    t('2600', 365, 64, 46, fill=ORANGE, face=p['bold'])
    logo(192, 372, 128)

    # Section rules
    for x in (365, 637, 909, 1328): ln((x, 88), (x, 990))
    ln((1537, 88), (1537, 990)); ln((1625, 30), (1625, 490)); ln((1770, 592), (1770, 990)); ln((1328, 582), (1537, 582))
    for x in (457, 672, 877, 1078, 1520, 1818): ln((x, 1052), (x, 1430))

    # Oscillators -------------------------------------------------------------------------
    VCO = [dict(c=235, kbd=157, sliders=[209, 261, 313], block=(130, 255), caps=[BLUE]*3, mid=[YELLOW, RED, YELLOW],
                fm=[('S/H|OUT', None), ('', 'adsr'), ('LFO', 'tri')], outs=[('OSC_OUT+0', 313, 420, 'SAW', 'saw', 382), ('OSC_OUT+1', 313, 490, 'PULSE', 'pulse', 522)]),
           dict(c=506, kbd=402, sliders=[455, 507, 559, 611], block=(377, 500), caps=[GREEN]*3, mid=[YELLOW, RED, BLUE, GREEN],
                fm=[('S/H|OUT', None), ('', 'adsr'), ('VCO|1', 'pulse'), ('NOISE|GEN', None)],
                outs=[('OSC_OUT+7', 559, 420, 'TRI', 'tri', 382), ('OSC_OUT+4', 611, 420, 'SAW', 'saw', 382), ('OSC_OUT+6', 559, 490, 'SINE', 'sine', 522), ('OSC_OUT+5', 611, 490, 'PULSE', 'pulse', 522)]),
           dict(c=778, kbd=674, sliders=[727, 779, 831, 883], block=(649, 772), caps=[BLUE]*3, mid=[GREEN, RED, GREEN, RED],
                fm=[('NOISE|GEN', None), ('', 'adsr'), ('VCO|2', 'sine'), ('', 'adsr')],
                outs=[('OSC_OUT+11', 831, 420, 'TRI', 'tri', 382), ('OSC_OUT+8', 883, 420, 'SAW', 'saw', 382), ('OSC_OUT+10', 831, 490, 'SINE', 'sine', 522), ('OSC_OUT+9', 883, 490, 'PULSE', 'pulse', 522)])]
    for i, v in enumerate(VCO):
        c, b = v['c'], i*7
        t('INITIAL OSCILLATOR FREQUENCY', c, 98, 12)
        for dx, top, bottom in [(-93, '10', '.03'), (-32, '100', '0.3'), (30, '1kHz', '3.0'), (93, '10 kHz', '30.')]:
            w = t(top, c+dx, 110, 10); ln((c+dx-w/2-2, 113.5), (c+dx+w/2+2, 113.5), w=1, color=WHITE); t(bottom, c+dx, 125, 10)
        hslider(f'OSC+{b}', c-113, c+113, 138, v['caps'][0])
        t('FINE TUNE', c, 179, 12); ln((c, 184), (c, 197), w=1.2, color=WHITE)
        hslider(f'OSC+{b+1}', c-113, c+113, 210, v['caps'][1])
        t('PULSE WIDTH', c, 252, 12)
        for dx, s in [(-93, '10%'), (0, '50%'), (93, '90%')]: t(s, c+dx, 266, 10)
        hslider(f'OSC+{b+2}', c-113, c+113, 284, v['caps'][2])
        if i:
            sx = c-104; tl('SYNC|ON', sx, 330, 11, lead=11); switch(f'SYNC+{i-1}', sx, 372, 'swi'); t('OFF', sx, 406, 10)
        x0, x1 = v['block']
        block(x0, 425, x1, 487, f'VOLTAGE|CONTROLLED|OSCILLATOR {i+1}', 15, 15.5)
        for id, x, y, title, kind, ty in v['outs']:
            jack(id, x, y, True); t(title, x, ty, 12); icon(kind, x, ty+12, 15, 5.5)
        t('OUTPUTS', (v['outs'][0][1]+v['outs'][-1][1])/2, 457 if i == 0 else 362, 11)
        if i == 0:
            arrow((x1, 440), (283, 440), (293, 428)); arrow((x1, 472), (283, 472), (293, 483))
        else:
            ox = v['outs'][0][1]; mid = ox+26
            arrow((x1, 432), (ox-19, 432)); arrow((x1, 482), (ox-19, 482))
            arrow((x1, 448), (mid, 448), (mid, 420), (mid+8, 420)); arrow((x1, 463), (mid, 463), (mid, 490), (mid+8, 490))
        # Modulation sliders and their normalled jacks.
        k = v['kbd']
        switch(f'OSC+{b+6}', k, 786); t('AUDIO', k, 748, 13); t('KYBD ON', k, 760, 9); t('KYBD OFF', k, 825, 9); t('LF', k, 839, 13)
        for y in ([663, 715] if i == 0 else [610, 663, 715]): dummy(k, y)
        entries = [x0+14+(x1-x0-28)*j/len(v['sliders']) for j in range(len(v['sliders'])+1)]
        arrow((k, 645 if i == 0 else 592), (entries[0], 488))
        ids = [f'FM_SH+{i}', f'FM_EG+{i}', f'OSC+{b+3}']+([f'OSC+{b+4}'] if i else [])
        ports = [f'FM_SH_IN+{i}', f'FM_EG_IN+{i}', f'OSC_IN+{i*4+1}']+([f'OSC_IN+{i*4+2}'] if i else [])
        for j, (sx, id, port, (title, kind), cap) in enumerate(zip(v['sliders'], ids, ports, v['fm'], v['mid'])):
            vslider(id, sx, 615, 845, cap); arrow((sx, 610), (entries[j+1], 488))
            norm_jack(port, sx, title or 'ADSR', kind)
        norm_jack(f'OSC_IN+{i*4}', k, 'KYBD|CV')
        bracket(k, v['sliders'][2], 876, 'FM CONTROL')
        if i: t('PWM', v['sliders'][3], 880, 11)
    for x in (481, 533, 585, 753, 805, 857, 1067, 1119, 1171, 1223): dummy(x, 578)

    # Filter --------------------------------------------------------------------------------
    t('INITIAL FILTER FREQUENCY', 1118, 98, 12)
    for x, s in [(1025, '10'), (1087, '100'), (1149, '1kHz'), (1210, '10 kHz')]: t(s, x, 110, 10)
    hslider('CUTOFF', 1005, 1232, 138, CLEAR)
    t('FINE TUNE', 1118, 179, 12); ln((1118, 184), (1118, 197), w=1.2, color=WHITE); hslider('FILTER_FINE', 1005, 1232, 210, CLEAR)
    t('RESONANCE', 1118, 252, 12); t('MIN', 1025, 266, 10); t('MAX', 1212, 266, 10); hslider('RES', 1005, 1232, 284, CLEAR)
    for y in (175, 248): dummy(962, y)
    for x in (1067, 1119, 1171, 1223): dummy(x, 367)
    tl('MODE|4012', 962, 330, 11, lead=11); switch('FILTER_MODE', 962, 372); t('4072', 962, 406, 10)
    block(1062, 404, 1228, 502, 'VOLTAGE|CONTROLLED|FILTER / RESONATOR|VCF', 14, 15)
    jack('FILTER_OUT', 1301, 455, True); t('OUTPUT', 1301, 486, 11); arrow((1228, 455), (1281, 455))
    sources = [('RING_LEVEL', 'RING_MIX_IN', 'RING|MOD', None, GREEN), ('OSC+5', 'OSC_IN+3', 'VCO|1', 'pulse', BLUE),
               ('OSC+12', 'OSC_IN+7', 'VCO|2', 'pulse', GREEN), ('OSC+19', 'OSC_IN+11', 'VCO|3', 'saw', BLUE),
               ('NOISE_LEVEL', 'NOISE_MIX_IN', 'NOISE|GEN', None, GREEN), ('KEYTRACK', 'FILTER_CV', 'KYBD|CV', None, CLEAR),
               ('FILTER_EG', 'FILTER_EG_IN', 'ADSR', 'adsr', RED), ('FILTER_FM', 'FILTER_FM_IN', 'VCO|2', 'sine', GREEN)]
    for j, (param, port, title, kind, cap) in enumerate(sources):
        x = 935+52.3*j
        vslider(param, x, 615, 845, cap); norm_jack(port, x, '' if title == 'ADSR' else title, kind)
        if j < 3: arrow((x, 610), (x, 435+16*j), (1062, 435+16*j))
        else: arrow((x, 610), (x, 540-8*abs(j-5)), (1092+22*(j-3), 495))
    bracket(935, 1144, 876, 'AUDIO'); bracket(1196, 1301, 876, 'CONTROL')

    # Envelopes -------------------------------------------------------------------------------
    for x, s in [(1353, 'ATTACK|TIME'), (1405, 'DECAY|TIME'), (1457, 'SUS|LEVEL'), (1510, 'REL|TIME')]: tl(s, x, 97, 12)
    for id, x in [('A', 1353), ('D', 1405), ('S', 1457), ('R', 1510)]:
        vslider(id, x, 118, 340, RED); icon('adsr', x, 352, 13, 5)
    block(1338, 362, 1526, 405, 'ADSR|ENVELOPE GENERATOR', 15, 15.5)
    t('x2', 1353, 423, 10); t('x1', 1376, 459, 10, anchor='start'); switch('TIME_FACTOR', 1353, 455, 'sw3'); tl('x0.5|TIME|FACTOR', 1353, 491, 10)
    jack('ADSR_GATE_IN', 1405, 455); t('GATE IN', 1405, 486, 11); arrow((1405, 437), (1405, 406))
    jack('ADSR_OUT', 1510, 455, True); t('OUTPUT', 1510, 486, 11); arrow((1510, 405), (1510, 436))
    ln((1380, 582), (1380, 545), (1405, 545), (1405, 500))

    for x, s in [(1353, 'ATTACK|TIME'), (1510, 'REL|TIME')]: tl(s, x, 598, 11)
    t('MANUAL', 1405, 606, 11); t('OUTPUT', 1457, 606, 11)
    vslider('AR_A', 1353, 615, 845, RED); vslider('AR_R', 1510, 615, 845, RED)
    icon('ar', 1353, 858, 13, 6); icon('ar', 1510, 858, 13, 6)
    switch('MANUAL', 1405, 636, 'button'); jack('AR_OUT', 1457, 636, True)
    block(1385, 697, 1478, 738, 'AR|ENV GEN', 15, 15.5)
    arrow((1405, 654), (1405, 696)); arrow((1457, 697), (1457, 656))
    switch('GATE_SELECT', 1405, 820, 'sw3')
    ln((1405, 738), (1405, 790)); dot(1380, 760); ln((1380, 760), (1405, 760)); arrow((1380, 760), (1380, 790), (1395, 805))
    t('x2', 1353, 886, 10); t('x1', 1376, 916, 10, anchor='start'); switch('AR_TIME_FACTOR', 1353, 912, 'sw3'); tl('x0.5', 1353, 946, 10)
    tl('TIME|FACTOR', 1353, 972, 11)
    norm_jack('ADSR_GATE', 1405, 'S/H|CLOCK'); ln((1405, 886), (1405, 852))
    jack('GATE', 1457, 905); t('GATE IN', 1457, 972, 11); icon('gate', 1457, 985, 15, 5.5)
    jack('TRIG', 1510, 905); t('TRIG IN', 1510, 972, 11); icon('trig', 1510, 985, 15, 5.5)
    arrow((1510, 886), (1510, 866), (1418, 866)); ln((1457, 886), (1457, 866))

    # Amplifier, mixer, reverb and outputs ---------------------------------------------------------
    tl('INITIAL|GAIN', 1562, 97, 12); vslider('VCA_BIAS', 1562, 118, 340, CLEAR); dummy(1605, 228)
    jack('VCA_OUT', 1562, 455, True); t('OUTPUT', 1562, 486, 11); arrow((1598, 496), (1598, 455), (1581, 455))
    block(1560, 499, 1678, 563, 'VOLTAGE|CONTROLLED|AMPLIFIER', 14, 15)
    for j, (param, port, title, kind, cap) in enumerate([('VCA_FILTER', 'VCA_IN', 'VCF', None, CLEAR), ('VCA_RING', 'VCA_RING_IN', 'RING|MOD', None, GREEN),
                                                       ('VCA_AR', 'AR_VCA_IN', '', 'ar', RED), ('VCA_ENV', 'VCA_CV', '', 'adsr', RED)]):
        x = 1562+52.3*j
        vslider(param, x, 615, 845, cap); norm_jack(port, x, title or ('AR' if kind == 'ar' else 'ADSR'), kind)
        arrow((x, 610), (x, 590), (1580+30*j, 560))
    bracket(1562, 1614, 876, 'AUDIO'); bracket(1667, 1719, 876, 'CNTRL'); t('LIN', 1656, 885, 8); t("EXP'L", 1730, 885, 8)

    jack('OUT_COPY', 1655, 100, True); jack('LEFT_OUT', 1722, 100, True, 'big')
    jack('RIGHT_OUT', 1797, 100, True, 'big'); jack('OUT_COPY+1', 1866, 100, True)
    ln((1673, 100), (1696, 100)); ln((1823, 100), (1848, 100))
    t('L OUTPUT', 1708, 151, 12); t('R OUTPUT', 1813, 151, 12)
    for x in (1655, 1866):
        ln((x-12, 151), (x+12, 151), (x, 129), (x-12, 151), w=1.6); ln((x, 151), (x, 212)); dot(x, 172)
    t('PAN', 1761, 157, 12)
    hslider('PAN', 1682, 1845, 172, RED); arrow((1682, 172), (1659, 172)); arrow((1845, 172), (1862, 172))
    ln((1762, 180), (1762, 192), (1895, 192), (1895, 358)); arrow((1895, 358), (1860, 358))
    jack('LEFT_IN', 1655, 228); tl('LEFT|INPUT', 1655, 258, 11); jack('RIGHT_IN', 1866, 228); tl('RIGHT|INPUT', 1866, 258, 11)
    vslider('REVERB_MIX', 1710, 245, 455, GREEN); vslider('REVERB_LEVEL_R', 1762, 245, 455, GREEN, ticks=0)
    ln((1710, 243), (1710, 228)); arrow((1710, 228), (1676, 228)); ln((1762, 243), (1762, 228)); arrow((1762, 228), (1845, 228)); dot(1797, 228)
    block(1700, 466, 1772, 488, 'REVERB', 14)
    jack('MIX_OUT', 1710, 516, True); tl('MIXER|OUT', 1710, 565, 10, lead=10); jack('REVERB_L', 1762, 516, True); tl('REVERB|OUT', 1762, 565, 10, lead=10)
    jack('PAN_IN', 1840, 358); ln((1840, 376), (1840, 408)); normal_mark(1840, 420); ln((1840, 455), (1840, 432))
    block(1808, 455, 1872, 478, 'MIXER', 14)
    for x in (1814, 1866):
        normal_mark(x, 508); ln((x, 478), (x, 496)); jack(f'MIX_DIRECT_IN+{0 if x == 1814 else 1}', x, 548)
    for x, param, port, title in [(1814, 'MIX_FILTER', 'MIX_FILTER_IN', 'VCF'), (1866, 'MIX_VCA', 'MIX_VCA_IN', 'VCA')]:
        vslider(param, x, 615, 845, CLEAR, ticks=0); norm_jack(port, x, title); arrow((x, 610), (x, 594))
    bracket(1814, 1866, 876, 'AUDIO')

    # Keyboard and portamento ---------------------------------------------------------------------
    t('TRIG OUT', 157, 1011, 11); icon('trig', 157, 1022, 15, 5); jack('KEY_TRIG_OUT', 157, 1045, True)
    tl('UPPER|VOICE', 209, 1009, 11, lead=11); jack('UPPER_OUT', 209, 1045, True)
    t('GATE OUT', 157, 1077, 11); icon('gate', 157, 1088, 15, 5); jack('KEY_GATE_OUT', 157, 1112, True)
    tl('KYBD|CV', 209, 1076, 11, lead=11); jack('KEY_OUT', 209, 1112, True)
    tl('KBD|IN', 104, 1009, 10, lead=10); jack('PITCH', 104, 1045)
    tl('UPPER|IN', 104, 1076, 10, lead=10); jack('UPPER_CV', 104, 1112)
    tl('VOICE|MODE', 266, 1021, 13, lead=13); t('MONO', 266, 1049, 10); switch('DUO', 266, 1078); t('DUO', 266, 1112, 10)
    vslider('GLIDE', 209, 1160, 1385, CLEAR, ticks=0); t('MAX', 192, 1176, 10, anchor='end'); t('MIN', 192, 1362, 10, anchor='end')
    for k in range(5): ln((227, 1182+45*k), (234, 1182+45*k), w=1.4, color='#ececec')
    t('ON', 151, 1236, 10); switch('GLIDE_ON', 151, 1266, 'swi'); t('OFF', 151, 1302, 10)
    t('MOMEN', 151, 1329, 10); switch('GLIDE_HOLD', 151, 1348, 'button')
    block(130, 1384, 232, 1407, 'PORTAMENTO', 13)
    t('REPEAT', 266, 1155, 13); t('KYBD', 266, 1167, 10); switch('REPEAT', 266, 1194, 'sw3i'); t('OFF', 285, 1198, 10, anchor='start'); t('AUTO', 266, 1230, 10)
    tl('TRIG|MODE', 266, 1278, 13, lead=13); t('SINGLE', 266, 1305, 10); switch('MULTI_TRIG', 266, 1336); t('MULT', 266, 1370, 10)

    # LFO and vibrato
    t('LFO', 323, 1011, 11); icon('tri', 323, 1022, 15, 5); jack('LFO_OUT', 323, 1045, True)
    tl('EXT VIB|IN', 376, 1009, 11, lead=11); jack('VIB_IN', 376, 1045)
    t('LFO', 323, 1077, 11); icon('pulse', 323, 1088, 15, 5); jack('LFO_OUT+1', 323, 1112, True)
    t('LFO', 368, 1077, 11); icon('sine', 388, 1074, 13, 5); t('DELAYED', 376, 1090, 10); jack('LFO_OUT+2', 376, 1112, True)
    ln((394, 1045), (429, 1045)); arrow((429, 1045), (429, 1150)); ln((360, 1112), (350, 1112), (350, 1060), (360, 1050))
    for id, x, s in [('LFO_RATE', 323, 'LFO|SPEED'), ('VIB_DELAY', 376, 'VIB|DELAY'), ('VIB_DEPTH', 429, 'VIB|DEPTH')]:
        vslider(id, x, 1160, 1385, YELLOW); tl(s, x, 1399, 11, lead=11)

    # Preamp and envelope follower
    block(462, 1062, 510, 1082, 'PREAMP', 10); t('GAIN', 481, 1143, 12)
    jack('PRE_OUT', 533, 1112, True); t('OUTPUT', 533, 1143, 11); ln((510, 1072), (533, 1072)); arrow((533, 1072), (533, 1093))
    vslider('PRE_GAIN', 481, 1160, 1385, CLEAR, ticks=0); t('MAX', 499, 1176, 10, anchor='start'); t('MIN', 499, 1362, 10, anchor='start')
    t('RANGE', 533, 1232, 13); t('x1000', 533, 1245, 9); switch('PRE_RANGE', 533, 1272, 'sw3'); t('x100', 550, 1276, 9, anchor='start'); t('x10', 533, 1304, 9)
    t('INPUT', 533, 1346, 11); jack('PRE_IN', 533, 1368)
    ln((533, 1387), (533, 1410), (481, 1410)); arrow((481, 1410), (481, 1391)); arrow((481, 1156), (481, 1083))
    block(555, 1060, 616, 1088, 'ENV|FOLLOWER', 10, 10)
    jack('FOLLOW_OUT', 637, 1112, True); t('OUTPUT', 637, 1143, 11); ln((616, 1074), (637, 1074)); arrow((637, 1074), (637, 1093))
    vslider('FOLLOW_GAIN', 585, 1160, 1385, RED); arrow((585, 1156), (585, 1089))
    block(617, 1310, 657, 1330, 'PREAMP', 9); arrow((637, 1330), (637, 1349)); jack('FOLLOW_IN', 637, 1368)
    ln((637, 1387), (637, 1410), (585, 1410)); arrow((585, 1410), (585, 1391))

    # Ring modulator
    block(683, 1062, 803, 1082, 'RING MOD', 14)
    jack('RING_OUT', 846, 1112, True); t('OUTPUT', 846, 1143, 11); ln((803, 1072), (846, 1072)); arrow((846, 1072), (846, 1093))
    for y in (1112, 1153, 1196): dummy(742, y)
    vslider('RING_X', 690, 1160, 1385, GREEN); vslider('RING_Y', 795, 1160, 1385, GREEN)
    arrow((690, 1156), (690, 1083)); arrow((795, 1156), (795, 1083))
    t('AUDIO', 742, 1236, 10); switch('RING_MODE', 742, 1266); t('DC', 742, 1302, 10)
    for x, port, s, kind, x0 in [(742, 'RING_X_IN', 'VCO 1', 'saw', 690), (846, 'RING_Y_IN', 'VCO 2', 'sine', 795)]:
        rect(x-23, 1310, x+23, 1336); t(s, x, 1321, 9); icon(kind, x, 1330, 15, 4.5)
        arrow((x, 1336), (x, 1349)); jack(port, x, 1368); ln((x, 1387), (x, 1410), (x0, 1410)); arrow((x0, 1410), (x0, 1391))

    # Noise generator
    block(885, 1062, 1030, 1082, 'NOISE GENERATOR', 14); dummy(957, 1112)
    jack('NOISE_OUT', 1050, 1112, True); t('OUTPUT', 1050, 1143, 11); ln((1030, 1072), (1050, 1072)); arrow((1050, 1072), (1050, 1093))
    t('COLOR', 930, 1153, 12); t('LEVEL', 983, 1153, 12)
    vslider('NOISE_COLOR', 930, 1160, 1385, GREEN, ticks=0, inverted=True)
    t('WHITE', 913, 1188, 10, anchor='end'); t('PINK', 913, 1276, 10, anchor='end'); tl('LOW|FREQ', 905, 1352, 10, lead=11)
    vslider('NOISE_LEVEL_OUT', 983, 1160, 1385, GREEN); t('MAX', 1013, 1188, 10, anchor='start'); t('MIN', 1013, 1362, 10, anchor='start')

    # Multiple
    t('MULT', 1176, 1031, 12); ln((1220, 1027), (1378, 1027))
    for j, x in enumerate((1220, 1272, 1325, 1378)): jack('MULT_IN' if j == 0 else f'MULT_OUT+{j-1}', x, 1027, j > 0)

    # Voltage processors
    block(1210, 1060, 1388, 1082, 'VOLTAGE PROCESSOR', 14)
    for j, (id, y, num, s) in enumerate([('VP_IN+0', 1148, '2', '-10V'), ('VP_IN+2', 1222, '4', 'KYBD CV'), ('VP_IN+4', 1295, '6', '+10V'), ('LAG_IN', 1368, '7', 'ENV FOLL')]):
        jack(id, 1110, y); t(s, 1110, 1124+73.3*j, 10); t(num, 1087, y+4, 11, anchor='end'); arrow((1129, y), (1152, y))
        hslider('LAG_TIME' if j == 3 else f'VP_GAIN+{j}', 1157, 1382, y, CLEAR)
    for id, y, num in [('VP_IN+1', 1110, '1'), ('VP_IN+3', 1185, '3'), ('VP_IN+5', 1258, '5')]:
        jack(id, 1425, y); t(num, 1402, y+4, 11, anchor='end')
    ln((1382, 1148), (1458, 1148)); ln((1443, 1110), (1458, 1110), (1458, 1210)); ln((1382, 1222), (1458, 1222), (1458, 1208))
    ln((1443, 1185), (1458, 1185)); dot(1458, 1148); dot(1458, 1185)
    ln((1458, 1222), (1492, 1222), (1492, 1213)); ln((1478, 1213), (1506, 1213), (1492, 1185), (1478, 1213), w=1.6)
    jack('VP_OUT', 1492, 1148, True); ln((1492, 1185), (1492, 1167)); t('INVERTER', 1467, 1234, 10)
    ln((1382, 1295), (1440, 1295)); arrow((1425, 1277), (1425, 1292)); dot(1425, 1295)
    ln((1440, 1281), (1440, 1309), (1466, 1295), (1440, 1281), w=1.6); arrow((1466, 1295), (1473, 1295))
    jack('VP_OUT+2', 1492, 1295, True); t('INVERTER', 1467, 1323, 10)
    ln((1382, 1368), (1410, 1368)); rect(1410, 1357, 1442, 1380, fill='none', stroke=LINE, w=1.4)
    t('LAG', 1426, 1373, 10); icon('lag', 1426, 1392, 15, 6); arrow((1442, 1368), (1473, 1368)); jack('LAG_OUT', 1492, 1368, True)
    t('INCREASE  LAG  TIME', 1270, 1393, 9); arrow((1170, 1389), (1205, 1389)); arrow((1335, 1389), (1370, 1389))

    # Sample and hold, clock and electronic switch
    block(1577, 1060, 1715, 1082, 'SAMPLE & HOLD', 14)
    ln((1577, 1071), (1553, 1071), (1553, 1093)); jack('SH_IN', 1553, 1112); ln((1553, 1131), (1553, 1138)); normal_mark(1553, 1148)
    rect(1533, 1163, 1573, 1196); t('NOISE', 1553, 1177, 10); t('GEN', 1553, 1189, 10)
    jack('CLOCK_OUT', 1553, 1220, True); tl('INT|CLK|OUT', 1574, 1203, 10, lead=10, anchor='start'); icon('pulse', 1586, 1240, 13, 4)
    jack('SH_OUT', 1553, 1295, True); tl('S/H|OUT', 1574, 1291, 10, lead=10, anchor='start')
    ln((1553, 1314), (1553, 1321)); normal_mark(1553, 1333); jack('CLOCK_IN', 1553, 1368)
    tl('EXT|CLK|IN', 1574, 1351, 10, lead=10, anchor='start'); icon('comb', 1586, 1388, 13, 4)
    ln((1530, 1240), (1530, 1418), (1600, 1418), (1600, 1083))
    dummy(1638, 1112); t('LEVEL', 1640, 1143, 12); t('RATE', 1692, 1143, 12)
    vslider('SH_LEVEL', 1640, 1160, 1385, YELLOW); vslider('CLOCK_RATE', 1692, 1160, 1385, YELLOW); icon('pulse', 1718, 1371, 13, 4)
    tl('ELEC|SWITCH', 1765, 1074, 11, lead=11)
    jack('SWITCH_A_IN', 1738, 1112); t('A', 1738, 1143, 11); jack('SWITCH_C_IN', 1790, 1112); t('B', 1790, 1143, 11)
    for x, light in [(1738, 'SWITCH_A_LIGHT'), (1790, 'SWITCH_B_LIGHT')]:
        ln((x, 1150), (x, 1164)); place('light', light, x, 1176); ln((x, 1188), (x, 1245))
    cx, cy = X(1765), Y(1245)
    add(f'<circle cx="{n(cx)}" cy="{n(cy)}" r="13" fill="none" stroke="{LINE}" stroke-width="1.5"/>', cy-13, cy+13)
    arrow((1738, 1245), (1758, 1245)); arrow((1790, 1245), (1772, 1245)); dot(1765, 1245, 2.2); arrow((1765, 1245), (1765, 1276))
    jack('SWITCH_B_OUT', 1765, 1295, True); t('C', 1765, 1327, 11)
    block(1738, 1335, 1792, 1366, 'INT|CLOCK', 11, 11); arrow((1738, 1352), (1730, 1352), (1730, 1265), (1752, 1255)); ln((1765, 1366), (1765, 1430))

    # Phones and power
    t('PHONES', 1866, 1087, 12); jack('PHONES_OUT', 1866, 1112, True); ln((1866, 1131), (1866, 1166))
    switch('PHONES_LEVEL', 1866, 1194, 'knob')
    for k, s in enumerate(['0', '2', '4', '6', '8', '10']):
        a = math.radians(225-54*k); r = 33
        t(s, 1866+math.cos(a)*r/.84*1.03, 1194-math.sin(a)*r/.817*1.0+4, 9)
        lx, ly = X(1866)+math.cos(a)*21, Y(1194)-math.sin(a)*21
        px_path([(lx, ly), (lx+math.cos(a)*4, ly-math.sin(a)*4)], 1.2, '#ececec', solid=True)
    place('light', 'ON_LIGHT', 1866, 1262); t('POWER', 1866, 1293, 17); t('ON', 1866, 1311, 10)
    switch('POWER', 1866, 1355, 'rocker')


def controls(p):
    """Control artwork used only by the Orange modules."""
    root = p['root']
    def asset(name, w, h, body):
        (root/f'res/{name}.svg').write_text(f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">{body}</svg>\n')
    silver = '<linearGradient id="s" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#ffffff"/><stop offset=".55" stop-color="#c2c2c0"/><stop offset="1" stop-color="#7a7a78"/></linearGradient>'
    nut = '<linearGradient id="n" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#5a5a5b"/><stop offset="1" stop-color="#1d1d1e"/></linearGradient>'
    asset('JackOrange', 26, 26, f'<defs>{silver}{nut}</defs><circle cx="13" cy="13" r="12.6" fill="#080808"/><circle cx="13" cy="13" r="11" fill="url(#n)"/>'
          '<circle cx="13" cy="13" r="7" fill="url(#s)"/><circle cx="13" cy="13" r="5" fill="#0a0a0a"/><circle cx="13" cy="13" r="3.4" fill="#000"/>')
    asset('BigJackOrange', 44, 44, f'<defs>{silver}</defs><circle cx="22" cy="22" r="21.6" fill="url(#s)" stroke="#555" stroke-width=".8"/>'
          '<circle cx="22" cy="22" r="16.5" fill="#9c9c9a"/><circle cx="22" cy="22" r="15" fill="url(#s)"/><circle cx="22" cy="22" r="11" fill="#141414"/><circle cx="22" cy="22" r="9" fill="#000"/>')
    for k, colour in enumerate(CAPS.values()):
        glow = f'<radialGradient id="g"><stop offset="0" stop-color="{colour}" stop-opacity=".8"/><stop offset=".55" stop-color="{colour}" stop-opacity=".35"/><stop offset="1" stop-color="{colour}" stop-opacity="0"/></radialGradient>'
        asset(f'OrangeCapV{k}', 20, 28, f'<defs>{glow}</defs><ellipse cx="10" cy="14" rx="10" ry="14" fill="url(#g)"/>'
              f'<rect x="6" y="7" width="8" height="14" rx="1.5" fill="{colour}"/><rect x="8" y="10" width="4" height="8" rx="1" fill="#ffffff" fill-opacity=".85"/>')
        asset(f'OrangeCapH{k}', 30, 20, f'<defs>{glow}</defs><ellipse cx="15" cy="10" rx="15" ry="10" fill="url(#g)"/>'
              f'<rect x="7" y="6" width="16" height="8" rx="1.5" fill="{colour}"/><rect x="10" y="8" width="10" height="4" rx="1" fill="#ffffff" fill-opacity=".85"/>')
    def lever(height, y):
        ridges = ''.join(f'<rect x="3" y="{y+2+2.4*k:.1f}" width="8" height="1.1" fill="#6a6a6a"/>' for k in range(4))
        return (f'<rect x=".5" y=".5" width="13" height="{height-1}" rx="2" fill="#101010" stroke="#000"/><rect x="3" y="3" width="8" height="{height-6}" rx="1" fill="#222"/>'
                f'<rect x="2.5" y="{y}" width="9" height="12" rx="1.5" fill="#3a3a3a" stroke="#000" stroke-width=".6"/>{ridges}')
    asset('OrangeSwitchUp', 14, 26, lever(26, 3)); asset('OrangeSwitchDown', 14, 26, lever(26, 11))
    for k, y in enumerate([17, 10, 3]): asset(f'OrangeRange{k}', 14, 32, lever(32, y))
    red = '<radialGradient id="r" cx=".4" cy=".35" r=".7"><stop offset="0" stop-color="#ff5a52"/><stop offset="1" stop-color="#a30f0f"/></radialGradient>'
    asset('OrangeButtonUp', 22, 22, f'<defs>{red}</defs><circle cx="11" cy="11" r="10.8" fill="#151515"/><circle cx="11" cy="11" r="8" fill="url(#r)"/>')
    asset('OrangeButtonDown', 22, 22, f'<defs>{red}</defs><circle cx="11" cy="11" r="10.8" fill="#151515"/><circle cx="11" cy="11" r="7.4" fill="#8a0c0c"/>')
    knurl = ''.join(f'<circle cx="{20+17*math.cos(a):.2f}" cy="{20+17*math.sin(a):.2f}" r="2.6" fill="#1a1a1a"/>' for a in [k*math.pi/9 for k in range(18)])
    asset('OrangeKnob', 40, 40, f'<defs><radialGradient id="k" cx=".4" cy=".35" r=".75"><stop offset="0" stop-color="#ffffff"/><stop offset=".6" stop-color="#bdbdbb"/>'
          f'<stop offset="1" stop-color="#6d6d6b"/></radialGradient></defs>{knurl}<circle cx="20" cy="20" r="17" fill="#1a1a1a"/><circle cx="20" cy="20" r="13" fill="url(#k)"/>'
          '<rect x="19" y="7.5" width="2" height="7" rx="1" fill="#2a2a2a"/>')
    def rocker(on):
        top, bottom = ('#2a2a2a', '#4a4a4a') if on else ('#4a4a4a', '#2a2a2a')
        return (f'<defs><linearGradient id="q" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="{top}"/><stop offset="1" stop-color="{bottom}"/></linearGradient></defs>'
                '<rect x=".5" y=".5" width="25" height="37" rx="3" fill="#0b0b0b" stroke="#000"/><rect x="4" y="4" width="18" height="30" rx="2" fill="url(#q)"/>')
    asset('OrangeRockerOff', 26, 38, rocker(False)); asset('OrangeRockerOn', 26, 38, rocker(True))
