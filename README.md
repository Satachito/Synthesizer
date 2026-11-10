# Pico Synth

Raspberry Pi Pico 2 W で動く、オールドシンセ風のシンセサイザー。機種を切り替えて使う。鍵盤は MIDI、つまみは Wi-Fi の画面。

## 機種

| 名前 | もとにした楽器 | 発音数 | 特徴 |
|---|---|---|---|
| MINI | Minimoog | モノ | 3 VCO、24dB ラダーフィルター、グライド |
| ACID | TB-303 | モノ | 強く弾く（ベロシティ 100 以上）とアクセント、重ねて弾くとスライド |
| JUNO | Juno-106 | 6 | DCO（ノコギリ + パルス + サブ）、HPF、コーラス I / II |
| PRO-5 | Prophet-5 | 5 | 2 VCO、ポリモジュレーション |
| MS | MS-20 | モノ | 発振する HPF と LPF、リングモジュレーター |
| SEM | Oberheim SEM | 4 | 12dB のマルチモードフィルター（LP〜ノッチ〜HP） |
| STRINGS | ストリングマシン | 8 | アンサンブル |

どの機種にもディレイ（エコー）が付いている。

## 配線

### Pico のピン

DAC は Digimin の DAC 版と同じ場所につなぐ。

| 物理ピン | GP | つなぐ先 |
|---|---|---|
| 1 | GP0 | DAC LCK |
| 2 | GP1 | DAC BCK |
| 3 | GND | DAC SCK |
| 4 | GP2 | DAC DIN |
| 7 | GP5 | MIDI IN 基板の出力（H11L1M の 4 番） |
| 8 | GND | MIDI IN 基板の GND |
| 36 | 3V3(OUT) | MIDI IN 基板の電源 |
| 38 | GND | DAC GND |
| 39 | VSYS | DAC VIN |

- DAC は PCM5102A モジュール。裏のジャンパーは H1 / H2 / H4 = L、H3 = H。
- USB を PC や iPad につなぐと USB-MIDI 機器として見える（Mac での名前は「Pico 2W virtual-cable」）。MIDI IN 基板なしでも DAW から鳴らせる。
- USB-MIDI 鍵盤を Pico に直接つなぐことはできない（Pico が USB の機器側なので）。鍵盤は DIN でつなぐか、PC を経由する。

### MIDI IN 基板（DIN → GP5）

フォトカプラ H11L1M と、秋月の 5P DIN コネクター DIP 化キット（AE-DIN-DIP）をユニバーサル基板に載せる。

| 部品 | 値 | 秋月の販売コード |
|---|---|---|
| フォトカプラ | H11L1M（DIP6） | 131184 |
| DIN ジャック | AE-DIN-DIP（キット） | 110261 |
| R1 | 220Ω | |
| R2 | 470Ω | |
| D1 | 1N4148 | |
| C1 | 0.1µF セラミック | |

回路:

| どこから | どこへ |
|---|---|
| DIN 4 番 | R1（220Ω）→ H11L1M 1 番（アノード） |
| DIN 5 番 | H11L1M 2 番（カソード） |
| D1 | 帯のある側を 1 番、反対側を 2 番（逆向きの電圧から LED を守る） |
| H11L1M 6 番（Vcc） | 3V3(OUT) |
| H11L1M 5 番（GND） | GND |
| H11L1M 4 番（出力） | GP5。R2（470Ω）で 3V3 にプルアップ |
| C1 | 6 番と 5 番の間 |

DIN の 1・2・3 番と H11L1M の 3 番はどこにもつながない。DIN の 2 番（シールド）を GND に落とさないのは MIDI IN の決まり。

配置（部品面から見た図。19 列 × 11 行）:

```
    1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
a   x  x  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·
b   ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·
c   ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·
d   ·  ·  ·  ·  ·  ·  3  ·  R1 ·  D1 p1 ·  ·  p6 R2 C1 ·  3V
e   ·  ·  ·  ·  ·  ·  5  ·  ·  ·  D1 p2 ·  ·  p5 ·  C1 ·  GN
f   ·  ·  ·  ·  ·  ·  2  ·  ·  ·  ·  p3 ·  ·  p4 R2 ·  ·  RX
g   ·  ·  ·  ·  ·  ·  4  ·  R1 ·  ·  ·  ·  ·  ·  ·  ·  ·  ·
h   ·  ·  ·  ·  ·  ·  1  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·
i   ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·
j   ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·
k   x  x  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·
```

| 図の記号 | 部品 | 差す穴 | 付け方 |
|---|---|---|---|
| 3 5 2 4 1 | 変換基板の 5 ピンの列（数字は DIN のピン番号） | 7d〜7h | ジャックの口を左（基板の外）へ |
| x | 変換基板の支えのピン | 1a・2a・1k・2k | 固定用。どこにもつながない |
| p1〜p6 | H11L1M の 1〜6 番ピン | 12d〜12f、15d〜15f | 切り欠きを a 行側に |
| R1 | 220Ω | 9d – 9g | 寝かせて縦に |
| D1 | 1N4148 | 11d（帯のある側）– 11e | 立てる |
| R2 | 470Ω | 16d – 16f | 立てる |
| C1 | 0.1µF | 17d – 17e | |
| 3V / GN / RX | Pico への線 | 19d = 3V3(OUT)、19e = GND、19f = GP5 | |

裏（ハンダ面）の配線:

| 行 | つなぐ範囲 | 中身 |
|---|---|---|
| d | 9 〜 12 | R1 → D1 → H11L1M 1 番 |
| d | 15 〜 19 | 3.3V |
| e | 7 〜 12 | DIN 5 番 → D1 → H11L1M 2 番 |
| e | 15 〜 19 | GND |
| f | 15 〜 19 | GP5 |
| g | 7 〜 9 | DIN 4 番 → R1 |

- 7d（DIN 3 番）と 9d の間、12 列と 15 列の間（IC の下）はつながない。
- 変換基板のピンの並び（口を左に向けて上から 3・5・2・4・1）は商品写真のシルクから読んだもの。実物のシルクで 7e が「5」、7g が「4」になっているか確かめる。逆でも D1 が守るので壊れず、受信しないだけ。
- 変換基板の支え用の穴は 1.27mm ずれて 2 組ある。「2.54」と書かれた側を使う。
- H11L1M は LED が光ると出力が Low になるので、UART にそのまま入れられる。電源は 3〜15V で、3.3V は定格内。

## ビルドと書き込み

```bash
arduino-cli compile --fqbn rp2040:rp2040:rpipico2w:opt=Optimize2 .
```

BOOTSEL を押しながら USB をつないでから:

```bash
arduino-cli upload --fqbn rp2040:rp2040:rpipico2w:opt=Optimize2 -p /Volumes/RP2350 .
```

`opt=Optimize2` は付ける（標準の -Os だと音の計算が遅くなる）。

## 使い方

1. Wi-Fi のアクセスポイント `Synth-XXXX` につなぐ（パスワードなし。付けるなら `Synthesizer.ino` の `WIFI_PASS`）。
2. ブラウザで `http://192.168.42.1/` を開く。
3. 上のボタンで機種を選び、つまみを上下にドラッグする。画面の鍵盤でも音が出る。
4. 「保存」を押すと、全機種の音色と今の機種がフラッシュに残る。押さなければ電源を切ると消える。

画面右上の「CPU」は音の計算の負荷。平均が 100% に近づくと音が途切れる。「最大」はつまみを動かした瞬間に 100% を超えることがあるが、一瞬なので途切れない。同じ数字が 1 秒ごとに USB シリアルにも出る。

実測（150MHz、最大発音数で和音）: JUNO 6 音 73%、STRINGS 8 音 76%、PRO-5 5 音 41%、SEM 4 音 35%、モノの機種は 15% 以下。余裕がほしいときは `--fqbn` に `,freq=200` を足してオーバークロックする。

## MIDI

### 機器名

USB でつなぐと、ドライバーなしの USB-MIDI 機器として見える。アプリで出力先を選ぶときの名前:

| 見える場所 | 名前 |
|---|---|
| Mac の MIDI の出力先（DAW やアプリの一覧） | `Pico 2W virtual-cable` |
| USB 機器としての製品名 | `Pico 2W`（メーカー名 Raspberry Pi） |

DIN の MIDI IN は機器名がなく、つないだ鍵盤やシーケンサーからそのまま受ける。

### 受けるメッセージ

チャンネルは見ない（どのチャンネルでも鳴る）。

| メッセージ | 動き |
|---|---|
| ノートオン / オフ、ピッチベンド（±2 半音） | 演奏 |
| プログラムチェンジ 0〜6 | 機種の切り替え |
| CC 1 | ビブラート |
| CC 5 | グライド |
| CC 7 | 音量 |
| CC 64 | サステインペダル |
| CC 71 / 74 | レゾナンス / カットオフ |
| CC 72 / 73 / 75 / 79 | リリース / アタック / ディケイ / サステイン |
| CC 76 / 77 | LFO の速さ / ビブラートの深さ |
| CC 91 / 93 | エコーの量 / コーラス |
| CC 120 / 123 | 全部の音を止める |

## アプリ

名前は 2026-11 時点の記憶によるもので、配布状況や価格は確かめていない。

### 弾くためのアプリ（MIDI を送る側）

| | アプリ | 用途 |
|---|---|---|
| Mac | VMPK（Virtual MIDI Piano Keyboard） | 画面の鍵盤と PC のキーボードで弾く。無料 |
| Mac | MIDI Monitor（Snoize） | 送受信している MIDI を見る。確認用。無料 |
| Mac | Reaper、Logic Pro、Ableton Live | 外部 MIDI トラックで打ち込んで鳴らす |
| iPad | AUM | 画面の鍵盤から外部へ MIDI を送る |
| iPad | Xequence 2、Cubasis、Logic Pro for iPad | 打ち込んで鳴らす |

- GarageBand（Mac / iPad）は外部の MIDI 機器へ送れないので使えない。
- iPad は USB-C でつなぐ（iPad では未確認）。Wi-Fi と USB は同時に使えるので、Wi-Fi の画面でつまみを操作しながら USB-MIDI のアプリで弾ける。
- アプリなしでも、Wi-Fi の画面の鍵盤で音は出る。HTTP 経由なので MIDI より反応が遅い。

### 聴き比べ用（同じ楽器を再現しているソフトシンセ）

| 機種 | アプリ |
|---|---|
| MINI | Moog「Minimoog Model D」（Mac / iPad）、Arturia「Mini V」 |
| ACID | Roland Cloud「TB-303」 |
| JUNO | Roland Cloud「JUNO-106」、TAL「TAL-U-NO-LX」（Juno-60） |
| PRO-5 | Arturia「Prophet-5 V」、u-he「Diva」 |
| MS | KORG「iMS-20」（iPad）、KORG Collection「MS-20」（Mac） |
| SEM | Arturia「SEM V」 |
| STRINGS | Arturia「Solina V」 |

これらは回路の細部まで計算していて PC の CPU を前提にしている。Pico Synth は 150MHz のマイコンで 6〜8 音を出すために計算を簡略化しているので、フィルターの鳴り方などは同じにはならない。

## ファイル

| ファイル | 中身 |
|---|---|
| `Synthesizer.ino` | 本体。Wi-Fi、Web、MIDI、保存（Core0）と I2S 出力（Core1） |
| `synth_engine.h` | 音の計算。Arduino に依存しない。実機では RAM に置く（フラッシュ上だと Wi-Fi 処理とキャッシュを取り合って数倍遅くなる） |
| `synth_params.h` | パラメータと機種の定義、機種ごとの初期の音色 |
| `web_ui.h` | 操作画面（HTML / JS）。機種ごとのつまみの並びはここの `PANELS` |
| `test/host_test.cpp` | エンジンを PC で鳴らして `test/out/*.wav` に書き出す |

PC でのテスト:

```bash
cd test && c++ -std=c++17 -O2 -I.. host_test.cpp -o host_test && ./host_test out/
```

機種を足すときは、`synth_params.h` の `MODELS` に 1 行と初期の音色、`web_ui.h` の `PANELS` につまみの並びを足す。

## ライセンス

MIT License。[LICENSE](LICENSE) を参照。
