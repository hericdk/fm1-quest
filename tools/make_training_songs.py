"""Three 64-bar songs for Piano Hero / FM Hero (the same files go to both games). Standard MIDI Files, one channel, notes F3..G5 (the 27 keys).
   usage: python3 tools/make_training_songs.py page/media/songs"""
import struct, sys, random, os

def vlq(n):
    b = [n & 0x7F]; n >>= 7
    while n: b.append((n & 0x7F) | 0x80); n >>= 7
    return bytes(reversed(b))

def write_mid(path, bpm, notes, tpq=480):
    """notes: [(start_in_16ths, pitch, length_in_16ths)]"""
    ev = []
    for s, p, l in notes:
        t0 = s * tpq // 4; t1 = t0 + max(1, int(l * tpq / 4 * 0.9))
        ev.append((t0, 1, p)); ev.append((t1, 0, p))
    ev.sort(key=lambda e: (e[0], e[1]))
    tr = bytearray(); tr += b'\x00\xFF\x51\x03' + struct.pack('>I', int(60000000 / bpm))[1:]
    last = 0
    for t, on, p in ev:
        tr += vlq(t - last) + bytes([0x90 if on else 0x80, p, 96 if on else 0]); last = t
    tr += b'\x00\xFF\x2F\x00'
    open(path, 'wb').write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, tpq) + b'MTrk' + struct.pack('>I', len(tr)) + bytes(tr))

BARS = 64
# --- 1) STEPS: C major, 88 bpm, quarter and half notes, stepwise; sections of 16 bars (A A B A')
def steps():
    C = [60, 62, 64, 65, 67, 69, 71, 72]
    A = [[60, 62, 64, 62], [64, 65, 67, 65], [67, 65, 64, 62], [60, 0, 60, 0]]
    B = [[67, 69, 71, 69], [67, 65, 64, 65], [62, 64, 65, 64], [62, 0, 67, 0]]
    notes = []
    for bar in range(BARS):
        sec = (bar // 16) % 4; phr = (bar % 16) // 4; sub = bar % 4
        src = B if sec == 2 else A
        row = src[sub]
        for i, p in enumerate(row):
            if p: notes.append((bar * 16 + i * 4, p + (12 if sec == 3 and p < 66 and phr == 2 else 0), 4 if i % 2 else 8 if sub == 3 else 4))
    return 88, notes
# --- 2) GROOVE: A minor pentatonic, 104 bpm, syncopated eighths
def groove():
    P = [57, 60, 62, 64, 67, 69, 72]
    riffs = [
        [(0, 0), (2, 2), (3, 3), (4, 4), (6, 3), (8, 2), (10, 1), (12, 0)],
        [(0, 3), (2, 4), (3, 5), (4, 4), (6, 2), (8, 3), (10, 2), (12, 1)],
        [(0, 4), (2, 5), (4, 6), (6, 5), (8, 4), (10, 3), (11, 2), (12, 1)],
        [(0, 2), (3, 3), (4, 2), (6, 1), (8, 0), (10, 1), (12, 2), (14, 3)],
    ]
    notes = []; rnd = random.Random(104)
    for bar in range(BARS):
        sec = bar // 16; r = riffs[(bar + sec) % 4]
        for s, d in r:
            idx = d + (1 if sec in (1, 3) and bar % 2 else 0)
            p = P[min(idx, len(P) - 1)]
            if sec == 3 and s in (0, 8): p = min(79, p + 12) if p + 12 <= 79 else p
            notes.append((bar * 16 + s, p, 2))
    return 104, notes
# --- 3) ARCADE: 124 bpm, sixteenth-note arpeggios over Am - F - C - G with jumps
def arcade():
    CH = [[57, 60, 64, 69], [53, 57, 60, 65], [60, 64, 67, 72], [55, 59, 62, 67]]
    PAT = [0, 1, 2, 3, 2, 1, 0, 1, 2, 3, 3, 2, 1, 2, 0, 1]
    notes = []
    for bar in range(BARS):
        sec = bar // 16; ch = CH[bar % 4]
        for i, k in enumerate(PAT):
            if sec == 0 and i % 2 == 1 and bar % 8 < 4: continue            # sparse intro
            if sec == 1 and i in (5, 13): continue
            p = ch[k] + (12 if sec == 2 and i in (3, 11) and ch[k] + 12 <= 79 else 0)
            if sec == 3 and i == 0: p = max(53, ch[0] - 12) if ch[0] - 12 >= 53 else ch[0]
            notes.append((bar * 16 + i, p, 1))
    return 124, notes

out = sys.argv[1] if len(sys.argv) > 1 else 'page/media/songs'
os.makedirs(out, exist_ok=True)
for name, fn in (('STEPS', steps), ('GROOVE', groove), ('ARCADE', arcade)):
    bpm, notes = fn()
    assert all(53 <= p <= 79 for _, p, _ in notes), name
    assert len(notes) <= 960, (name, len(notes))
    path = f'{out}/{name}.mid'; write_mid(path, bpm, notes)
    print(name, bpm, 'bpm', len(notes), 'notes', os.path.getsize(path), 'B file', 256 + 4 * len(notes), 'B in the device')
