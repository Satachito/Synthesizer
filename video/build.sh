#!/bin/zsh
# YouTube 用の動画（1920x1080・30fps）を組み立てる。macOS 専用（字幕を AppKit で描くため）
#
#   video/build.sh <画面録画.mp4> <写真> [出力.mp4]
#
#   画面録画: iPad（2732x2048）で操作画面の「全機種デモ」を録ったもの
#   写真:     Pico と DAC を写した 16:9 前後の写真
#   出力:     省略すると video/PicoSynth_YouTube.mp4
#
# 必要なもの: ffmpeg（Homebrew の ffmpeg で可）、Xcode のコマンドラインツール（swift と c++）
# 途中のファイルは video/work/ に置く（.gitignore 済み）
#
# 構成: タイトル 4.5 秒 → 写真 8 秒 → 配線 7 秒 → 書き込み 5 秒 → 録画 → できること 7 秒 → 作ってみませんか 7.9 秒
# 録画以外の部分の音は、エンジンを PC で鳴らしたデモ（test/out/demo_*.wav）を使う
set -e
if [ $# -lt 2 ]; then
  sed -n '3,9p' $0
  exit 1
fi
REC=${1:a}
PHOTO=${2:a}
HERE=${0:a:h}
ROOT=${HERE:h}
OUT=${3:-$HERE/PicoSynth_YouTube.mp4}
OUT=${OUT:a}
WORK=$HERE/work
P=$WORK/png
O=$WORK/seg
mkdir -p $P $O

# ---- 録画に合わせて変えるところ ----
# 録画のどこからどこまで使うか（秒）。find_switches.py で機種が切り替わる時刻を調べて決める
REC_START=1.2
REC_LEN=46.6
# 録画から 16:9 に切り出す範囲（iPad の画面の上のほうに操作画面がある）
REC_CROP="2732:1537:0:140"
# 機種ごとの字幕を出す時刻（切り出したあとの秒）。「開始 終了」の順
CAP_HEAD="0.6 5.0"     # 「iPad のブラウザで操作中」
CAP_ACID="0.4 5.2"
CAP_JUNO="5.6 11.8"
CAP_PRO5="12.2 18.3"
CAP_MS="18.7 24.0"
CAP_SEM="24.4 31.4"
CAP_STRINGS="31.8 40.5"
CAP_MINI="40.9 46.6"
# ------------------------------------

V="-c:v libx264 -preset medium -crf 18 -pix_fmt yuv420p -r 30 -an"
fadein() { echo "format=rgba,fade=t=in:st=$1:d=0.4:alpha=1" }
fio() { echo "format=rgba,fade=t=in:st=$1:d=0.35:alpha=1,fade=t=out:st=$2:d=0.35:alpha=1" }

echo "字幕を描く"
swift $HERE/cards.swift $P | tail -1

echo "デモの音を用意する"
WAV=$ROOT/test/out
if [ ! -f $WAV/demo_tour.wav ]; then
  mkdir -p $WAV
  c++ -std=c++17 -O2 -I$ROOT $ROOT/test/host_test.cpp -o $WORK/host_test
  $WORK/host_test $WAV > /dev/null
fi

echo "写真を 1920x1080 にする"
ffmpeg -v error -y -i $PHOTO -vf "scale=1920:1080:force_original_aspect_ratio=increase,crop=1920:1080" $WORK/photo1080.png
ffmpeg -v error -y -i $WORK/photo1080.png -vf "boxblur=24:2,eq=brightness=-0.08" $WORK/photo_blur.png

echo "場面を作る"
# 1. タイトル（ぼかした写真の上に）
ffmpeg -v error -y -loop 1 -t 4.5 -i $WORK/photo_blur.png -loop 1 -t 4.5 -i $P/title.png -filter_complex \
 "[1]$(fadein 0.4)[t];[0][t]overlay,fade=t=in:st=0:d=0.5,fade=t=out:st=4.1:d=0.4" $=V $O/1.mp4
# 2. 写真（ゆっくり寄りながら、部品の字幕を順に出す）
ffmpeg -v error -y -loop 1 -t 8 -i $WORK/photo1080.png -loop 1 -t 8 -i $P/photo_head.png -loop 1 -t 8 -i $P/photo_dac.png \
 -loop 1 -t 8 -i $P/photo_pico.png -loop 1 -t 8 -i $P/photo_wire.png -filter_complex \
 "[0]scale=3840:2160,zoompan=z='1+0.07*on/240':x='iw/2-iw/zoom/2':y='ih/2-ih/zoom/2':d=1:s=1920x1080:fps=30[b];\
  [1]$(fadein 0.2)[h];[2]$(fadein 1.0)[d];[3]$(fadein 2.8)[p];[4]$(fadein 4.6)[w];\
  [b][h]overlay[x1];[x1][d]overlay[x2];[x2][p]overlay[x3];[x3][w]overlay,fade=t=in:st=0:d=0.3,fade=t=out:st=7.6:d=0.4" $=V $O/2.mp4
# 3. 配線 / 4. 書き込み
ffmpeg -v error -y -loop 1 -t 7 -i $P/wiring.png -vf "format=rgb24,fade=t=in:st=0:d=0.3,fade=t=out:st=6.6:d=0.4" $=V $O/3.mp4
ffmpeg -v error -y -loop 1 -t 5 -i $P/software.png -vf "format=rgb24,fade=t=in:st=0:d=0.3,fade=t=out:st=4.6:d=0.4" $=V $O/4.mp4
# 5. 録画
L=$REC_LEN
ffmpeg -v error -y -ss $REC_START -t $L -i $REC \
 -loop 1 -t $L -i $P/rec_head.png -loop 1 -t $L -i $P/rec_acid.png -loop 1 -t $L -i $P/rec_juno.png -loop 1 -t $L -i $P/rec_pro5.png \
 -loop 1 -t $L -i $P/rec_ms.png -loop 1 -t $L -i $P/rec_sem.png -loop 1 -t $L -i $P/rec_strings.png -loop 1 -t $L -i $P/rec_mini.png \
 -filter_complex \
 "[0:v]crop=$REC_CROP,scale=1920:1080:flags=lanczos,fps=30[r];\
  [1]$(fio ${=CAP_HEAD})[c1];[2]$(fio ${=CAP_ACID})[c2];[3]$(fio ${=CAP_JUNO})[c3];[4]$(fio ${=CAP_PRO5})[c4];\
  [5]$(fio ${=CAP_MS})[c5];[6]$(fio ${=CAP_SEM})[c6];[7]$(fio ${=CAP_STRINGS})[c7];[8]$(fio ${=CAP_MINI})[c8];\
  [r][c1]overlay[x1];[x1][c2]overlay[x2];[x2][c3]overlay[x3];[x3][c4]overlay[x4];[x4][c5]overlay[x5];\
  [x5][c6]overlay[x6];[x6][c7]overlay[x7];[x7][c8]overlay,fade=t=in:st=0:d=0.3,fade=t=out:st=$((L - 0.5)):d=0.5" $=V $O/5.mp4
# 6. できること / 7. 作ってみませんか
ffmpeg -v error -y -loop 1 -t 7 -i $P/features.png -vf "format=rgb24,fade=t=in:st=0:d=0.3,fade=t=out:st=6.6:d=0.4" $=V $O/6.mp4
ffmpeg -v error -y -loop 1 -t 7.9 -i $P/cta.png -vf "format=rgb24,fade=t=in:st=0:d=0.4,fade=t=out:st=6.9:d=1.0" $=V $O/7.mp4

echo "音を作る"
# 前半はデモ（全機種）、録画の部分は録画の音、後半は PRO-5 と STRINGS のデモ。どれも -16 LUFS にそろえる
N="loudnorm=I=-16:TP=-1.5:LRA=11,aresample=48000"
ffmpeg -v error -y -t 24.5 -i $WAV/demo_tour.wav -af "$N,afade=t=in:d=0.3,afade=t=out:st=23:d=1.5" -ac 2 $O/a1.wav
ffmpeg -v error -y -ss $REC_START -t $L -i $REC -vn -af "$N,afade=t=in:d=0.2,afade=t=out:st=$((L - 1.0)):d=1.0" -ac 2 $O/a2.wav
ffmpeg -v error -y -t 7 -i "$WAV/demo_PRO-5.wav" -af "$N,afade=t=in:d=0.2,afade=t=out:st=6.2:d=0.8" -ac 2 $O/a3.wav
ffmpeg -v error -y -t 7.9 -i $WAV/demo_STRINGS.wav -af "$N,afade=t=in:d=0.3,afade=t=out:st=5.4:d=2.5" -ac 2 $O/a4.wav

echo "つなぐ"
printf "file '%s'\n" $O/1.mp4 $O/2.mp4 $O/3.mp4 $O/4.mp4 $O/5.mp4 $O/6.mp4 $O/7.mp4 > $O/v.txt
printf "file '%s'\n" $O/a1.wav $O/a2.wav $O/a3.wav $O/a4.wav > $O/a.txt
ffmpeg -v error -y -f concat -safe 0 -i $O/v.txt -c copy $O/video.mp4
ffmpeg -v error -y -f concat -safe 0 -i $O/a.txt -c copy $O/audio.wav
ffmpeg -v error -y -i $O/video.mp4 -i $O/audio.wav -c:v copy -c:a aac -b:a 192k -shortest -movflags +faststart $OUT
echo "できた: $OUT"
