// 動画用の字幕・カードを 1920x1080 の PNG で書き出す（透明部分あり）。build.sh から呼ばれる
// usage: swift cards.swift <出力フォルダ>
// 文字は macOS のヒラギノ角ゴシックと Menlo で描く（AppKit を使うので macOS 専用）
import AppKit

let outDir = CommandLine.arguments[1]
let W: CGFloat = 1920, H: CGFloat = 1080
let amber = NSColor(red: 0.91, green: 0.69, blue: 0.38, alpha: 1)
let ink = NSColor(red: 0.94, green: 0.91, blue: 0.84, alpha: 1)
let dim = NSColor(red: 0.66, green: 0.62, blue: 0.54, alpha: 1)
let bg = NSColor(red: 0.07, green: 0.07, blue: 0.075, alpha: 1)
let red = NSColor(red: 0.85, green: 0.22, blue: 0.22, alpha: 1)

func font(_ w: Int, _ size: CGFloat) -> NSFont { NSFont(name: "HiraginoSans-W\(w)", size: size)! }
func mono(_ size: CGFloat) -> NSFont { NSFont(name: "Menlo-Bold", size: size) ?? NSFont(name: "Menlo-Regular", size: size)! }

enum Align { case left, center, right }

// 上から y の位置（AppKit は下から数えるので変換する）に文字を置く。戻り値は描いた幅
@discardableResult
func text(_ s: String, _ f: NSFont, _ c: NSColor, x: CGFloat, y: CGFloat, align: Align = .left, kern: CGFloat = 0, shadow: Bool = false) -> CGFloat {
  var attrs: [NSAttributedString.Key: Any] = [.font: f, .foregroundColor: c, .kern: kern]
  if shadow {
    let sh = NSShadow()
    sh.shadowColor = NSColor(white: 0, alpha: 0.85)
    sh.shadowBlurRadius = 18
    sh.shadowOffset = NSSize(width: 0, height: -3)
    attrs[.shadow] = sh
  }
  let a = NSAttributedString(string: s, attributes: attrs)
  let sz = a.size()
  var px = x
  if align == .center { px = x - sz.width / 2 }
  if align == .right { px = x - sz.width }
  a.draw(at: NSPoint(x: px, y: H - y - sz.height))
  return sz.width
}

func rect(_ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat, _ c: NSColor, radius: CGFloat = 0) {
  c.setFill()
  NSBezierPath(roundedRect: NSRect(x: x, y: H - y - h, width: w, height: h), xRadius: radius, yRadius: radius).fill()
}

func render(_ name: String, _ draw: () -> Void) {
  let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: Int(W), pixelsHigh: Int(H), bitsPerSample: 8, samplesPerPixel: 4,
                             hasAlpha: true, isPlanar: false, colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0)!
  NSGraphicsContext.saveGraphicsState()
  NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
  NSColor.clear.setFill()
  NSRect(x: 0, y: 0, width: W, height: H).fill()
  draw()
  NSGraphicsContext.restoreGraphicsState()
  try! rep.representation(using: .png, properties: [:])!.write(to: URL(fileURLWithPath: "\(outDir)/\(name).png"))
  print("wrote", name)
}

// 画面下の字幕帯（左に色の線、大きい見出し + 小さい説明）
func lowerThird(_ title: String, _ sub: String, accent: NSColor = amber, y: CGFloat = 860) {
  let tf = font(8, 64), sf = font(6, 34)
  let tw = NSAttributedString(string: title, attributes: [.font: tf, .kern: 4]).size().width
  let sw = NSAttributedString(string: sub, attributes: [.font: sf]).size().width
  let w = max(tw, sw) + 120
  rect(80, y, w, 150, NSColor(white: 0, alpha: 0.72), radius: 10)
  rect(80, y, 12, 150, accent)
  text(title, tf, ink, x: 128, y: y + 14, kern: 4)
  text(sub, sf, amber, x: 130, y: y + 94)
}

// 好きな位置に置く字幕。見出し + 日本語 + 英語（英語は少し小さく、薄い色）
func boxCaption(_ title: String, _ ja: String, _ en: String, x: CGFloat, y: CGFloat, size: CGFloat = 56, alignRight: Bool = false) {
  let tf = font(8, size), jf = font(6, size * 0.55), ef = font(5, size * 0.46)
  let tw = NSAttributedString(string: title, attributes: [.font: tf, .kern: 2]).size().width
  let jw = NSAttributedString(string: ja, attributes: [.font: jf]).size().width
  let ew = NSAttributedString(string: en, attributes: [.font: ef]).size().width
  let w = max(tw, jw, ew) + 90, h = size * 2.95
  let bx = alignRight ? x - w : x
  rect(bx, y, w, h, NSColor(white: 0, alpha: 0.74), radius: 10)
  rect(bx, y, 10, h, amber)
  text(title, tf, ink, x: bx + 40, y: y + size * 0.2, kern: 2)
  text(ja, jf, amber, x: bx + 42, y: y + size * 1.4)
  text(en, ef, dim, x: bx + 42, y: y + size * 2.12)
}

// 1. タイトル（ぼかした写真の上に重ねる）
render("title") {
  rect(0, 0, W, H, NSColor(white: 0, alpha: 0.35))
  text("Raspberry Pi Pico 2 W", font(6, 52), amber, x: W / 2, y: 250, align: .center, kern: 2, shadow: true)
  text("オールドシンセを作ろう", font(9, 132), ink, x: W / 2, y: 340, align: .center, kern: 6, shadow: true)
  text("Build a vintage-style synthesizer", font(7, 64), ink, x: W / 2, y: 520, align: .center, kern: 1, shadow: true)
  text("部品 2 つ・ジャンパー線 6 本・7 機種を切り替え", font(6, 42), ink, x: W / 2, y: 660, align: .center, shadow: true)
  text("2 parts · 6 jumper wires · 7 switchable models", font(5, 36), dim, x: W / 2, y: 724, align: .center, shadow: true)
}

// 2. 写真の字幕
render("photo_head") {
  rect(0, 0, W, 170, NSColor(white: 0, alpha: 0.6))
  text("必要なのはこれだけ", font(8, 66), ink, x: W / 2, y: 20, align: .center, kern: 6)
  text("That's all you need", font(6, 38), amber, x: W / 2, y: 106, align: .center, kern: 1)
}
// 写真では DAC が x 580〜800、Pico が x 1000〜1250。字幕はその外側の空いているところに置く
render("photo_dac") { boxCaption("PCM5102A", "DAC モジュール・音を出す", "DAC module for audio out", x: 560, y: 680, size: 58, alignRight: true) }
render("photo_pico") { boxCaption("Pico 2 W", "ピンヘッダー付きの WH が便利", "The WH (with headers) is easiest", x: 1300, y: 680, size: 58) }
render("photo_wire") { boxCaption("ジャンパー線 6 本", "メス–メスでつなぐだけ", "6 female-to-female jumper wires", x: 1300, y: 300, size: 58) }

// 3. 配線表（不透明）
render("wiring") {
  rect(0, 0, W, H, bg)
  let w1 = text("配線", font(8, 76), ink, x: 140, y: 70, kern: 8)
  text("Wiring", font(7, 50), amber, x: 140 + w1 + 30, y: 92)
  text("PCM5102A  →  Pico 2 W", font(6, 38), dim, x: W - 140, y: 100, align: .right)
  let rows: [(String, String, String)] = [
    ("LCK", "GP0", "1"), ("BCK", "GP1", "2"), ("SCK", "GND", "3"),
    ("DIN", "GP2", "4"), ("GND", "GND", "38"), ("VIN", "VSYS", "39"),
  ]
  var y: CGFloat = 220
  for (i, r) in rows.enumerated() {
    rect(140, y - 18, W - 280, 104, NSColor(white: i % 2 == 0 ? 0.13 : 0.10, alpha: 1), radius: 8)
    text(r.0, mono(60), ink, x: 220, y: y)
    text("→", font(6, 56), dim, x: 520, y: y + 2)
    text(r.1, mono(60), amber, x: 660, y: y)
    text("物理ピン \(r.2)", font(6, 40), dim, x: 1040, y: y + 2)
    text("pin \(r.2)", font(5, 30), dim, x: 1040, y: y + 50)
    y += 116
  }
  text("DAC 裏のジャンパー: H1・H2・H4 = L、H3 = H", font(6, 34), dim, x: W / 2, y: 935, align: .center)
  text("Solder jumpers on the back of the DAC: H1 / H2 / H4 = L, H3 = H", font(5, 30), dim, x: W / 2, y: 985, align: .center)
}

// 4. 書き込みと操作
render("software") {
  rect(0, 0, W, H, bg)
  let steps: [(String, String, String, String, String)] = [
    ("1", "書き込む", "Flash", "Arduino IDE / arduino-cli（ボード: Raspberry Pi Pico 2W）", "with Arduino IDE or arduino-cli (board: Raspberry Pi Pico 2W)"),
    ("2", "Wi-Fi につなぐ", "Join Wi-Fi", "スマホや iPad で「Synth-XXXX」を選ぶ", "pick \"Synth-XXXX\" on your phone or iPad"),
    ("3", "ブラウザで開く", "Open the browser", "192.168.42.1 がそのまま操作画面になる", "192.168.42.1 is the control panel"),
  ]
  var y: CGFloat = 170
  for s in steps {
    rect(160, y, 130, 130, amber, radius: 65)
    text(s.0, font(9, 80), bg, x: 225, y: y + 14, align: .center)
    let w = text(s.1, font(8, 66), ink, x: 340, y: y - 8)
    text(s.2, font(7, 44), amber, x: 340 + w + 28, y: y + 10)
    text(s.3, font(6, 36), ink, x: 344, y: y + 80)
    text(s.4, font(5, 30), dim, x: 344, y: y + 128)
    y += 270
  }
}

// 5. 録画に重ねる字幕
render("rec_head") {  // 右下（録画の下のほうは空いている）
  let w: CGFloat = 660
  rect(W - 60 - w, 920, w, 120, NSColor(white: 0, alpha: 0.74), radius: 10)
  rect(W - 60 - w, 920, 12, 120, red)
  text("iPad のブラウザで操作中", font(8, 40), ink, x: W - 60 - w + 40, y: 936)
  text("Controlled from the iPad's browser", font(5, 30), dim, x: W - 60 - w + 42, y: 992)
}
let modelCaps: [(String, String, String, String)] = [
  ("rec_acid", "ACID", "TB-303 風・アクセントとスライド", "TB-303 style · accent and slide"),
  ("rec_juno", "JUNO", "Juno 風・DCO + コーラス・6 音", "Juno style · DCO + chorus · 6 voices"),
  ("rec_pro5", "PRO-5", "Prophet-5 風・ポリモジュレーション・5 音", "Prophet-5 style · poly-mod · 5 voices"),
  ("rec_ms", "MS", "MS-20 風・発振する HPF と LPF", "MS-20 style · screaming HPF and LPF"),
  ("rec_sem", "SEM", "Oberheim SEM 風・12dB マルチモード・4 音", "Oberheim SEM style · 12 dB multimode · 4 voices"),
  ("rec_strings", "STRINGS", "ストリングマシン風・アンサンブル・8 音", "String machine style · ensemble · 8 voices"),
  ("rec_mini", "MINI", "Minimoog 風・3 VCO + ラダーフィルター", "Minimoog style · 3 VCOs + ladder filter"),
]
for m in modelCaps { render(m.0) { boxCaption(m.1, m.2, m.3, x: 60, y: 912, size: 48) } }

// 6. できること
render("features") {
  rect(0, 0, W, H, bg)
  let w1 = text("できること", font(8, 76), ink, x: 140, y: 70, kern: 8)
  text("Features", font(7, 50), amber, x: 140 + w1 + 30, y: 92)
  let items: [(String, String, String, String)] = [
    ("7 機種", "7 models", "MINI / ACID / JUNO / PRO-5 / MS / SEM / STRINGS", ""),
    ("最大 8 音", "Up to 8 voices", "ポリの機種は和音が弾ける", "Play chords on the polyphonic models"),
    ("MIDI", "MIDI", "USB-MIDI と DIN の MIDI IN（フォトカプラ 1 個）", "USB-MIDI and DIN MIDI IN (one optocoupler)"),
    ("プリセット 32 個", "32 presets", "家で音を作って、ライブはプログラムチェンジで呼び出す", "Make sounds at home, recall them on stage by program change"),
    ("自動デモ", "Auto demo", "鍵盤がなくても音作りを確かめられる", "Check your sound without a keyboard"),
  ]
  var y: CGFloat = 220
  for it in items {
    rect(140, y + 22, 14, 14, amber, radius: 7)
    text(it.0, font(8, 52), amber, x: 190, y: y - 4)
    text(it.1, font(6, 30), dim, x: 192, y: y + 62)
    text(it.2, font(6, 36), ink, x: 720, y: y + 4)
    if !it.3.isEmpty { text(it.3, font(5, 30), dim, x: 720, y: y + 56) }
    y += 150
  }
}

// 7. 作ってみませんか？
render("cta") {
  rect(0, 0, W, H, bg)
  text("作ってみませんか？", font(9, 124), ink, x: W / 2, y: 190, align: .center, kern: 6)
  text("Why not build one?", font(7, 62), amber, x: W / 2, y: 360, align: .center, kern: 1)
  rect(W / 2 - 560, 500, 1120, 120, NSColor(white: 0.14, alpha: 1), radius: 14)
  text("github.com/Satachito/Synthesizer", mono(56), amber, x: W / 2, y: 526, align: .center)
  text("配線・書き込み方・使い方は README に。コードはすべて MIT ライセンスで公開", font(6, 36), dim, x: W / 2, y: 690, align: .center)
  text("Wiring, flashing and usage are in the README. All code is open source under the MIT License.", font(5, 30), dim, x: W / 2, y: 745, align: .center)
}
