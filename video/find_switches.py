#!/usr/bin/env python3
# 画面録画のどこで機種が切り替わったかを調べる（build.sh の時刻を決めるため）
#
#   python3 video/find_switches.py <画面録画.mp4>
#
# 操作画面の上のタブのうち、どれが明るい（選ばれている）かを 0.1 秒ごとに見る。
# 赤い「デモ停止」ボタンが出ているかも見る。iPad（2732x2048）で英語表示の画面を録ったときの位置に合わせてある。
# 別の端末や日本語表示で録ったときは、XS（タブの横位置）と DEMO_X を合わせ直す。
import subprocess
import sys

if len(sys.argv) < 2:
    print(__doc__ if __doc__ else 'usage: find_switches.py <recording.mp4>')
    sys.exit(1)

W, H, FPS = 683, 512, 10  # 1/4 に縮めて調べる
NAMES = ['MINI', 'ACID', 'JUNO', 'PRO-5', 'MS', 'SEM', 'STRINGS']
XS = [86, 115, 146, 179, 208, 234, 270]  # 縮めた画面でのタブの中心
HW = [8, 8, 8, 9, 6, 7, 14]              # タブの半分の幅
TAB_Y = (48, 58)
DEMO_X = (470, 560)                       # 「Demo」「Stop demo」ボタンのあたり

raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', sys.argv[1], '-vf', f'fps={FPS},scale={W}:{H}',
                      '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-'], check=True, capture_output=True).stdout
fs = W * H * 3
n = len(raw) // fs


def brightness(f, x0, x1, y0, y1):
    tot = c = 0
    for y in range(y0, y1):
        o = f * fs + (y * W + x0) * 3
        row = raw[o:o + (x1 - x0) * 3]
        tot += sum(row)
        c += len(row)
    return tot / c


def redness(f, x0, x1, y0, y1):
    best = 0
    for y in range(y0, y1, 2):
        for x in range(x0, x1, 2):
            o = f * fs + (y * W + x) * 3
            r, g, b = raw[o], raw[o + 1], raw[o + 2]
            best = max(best, r - (g + b) / 2)
    return best


prev = None
for f in range(n):
    ls = [brightness(f, x - h, x + h, *TAB_Y) for x, h in zip(XS, HW)]
    i = max(range(len(NAMES)), key=lambda k: ls[k])
    demo = redness(f, *DEMO_X, *TAB_Y) > 80
    state = (NAMES[i], demo)
    if state != prev:
        print(f'{f / FPS:5.1f}s  {NAMES[i]:8s} demo={"on" if demo else "off"}')
        prev = state
print(f'({n / FPS:.1f}s)')
