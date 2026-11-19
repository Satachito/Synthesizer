// YouTube のサムネイル（1280x720 の PNG）を描く。macOS 専用（AppKit）
//   swift video/thumbnail.swift <写真> <出力.png>
// 左に大きな文字、右に写真、下に 7 機種の名前を各機種の操作画面の色で並べる
import AppKit

let args = CommandLine.arguments
guard args.count >= 3, let photo = NSImage(contentsOfFile: args[1]) else {
  print("usage: swift thumbnail.swift <photo> <out.png>")
  exit(1)
}
let W: CGFloat = 1280, H: CGFloat = 720
let ink = NSColor(red: 0.97, green: 0.94, blue: 0.87, alpha: 1)
let amber = NSColor(red: 0.95, green: 0.70, blue: 0.33, alpha: 1)
let bg = NSColor(red: 0.06, green: 0.06, blue: 0.065, alpha: 1)

func font(_ w: Int, _ size: CGFloat) -> NSFont { NSFont(name: "HiraginoSans-W\(w)", size: size)! }

// y は上から数える
@discardableResult
func text(_ s: String, _ f: NSFont, _ c: NSColor, x: CGFloat, y: CGFloat, kern: CGFloat = 0, stroke: Bool = false, center: Bool = false) -> NSSize {
  var attrs: [NSAttributedString.Key: Any] = [.font: f, .foregroundColor: c, .kern: kern]
  let sh = NSShadow()
  sh.shadowColor = NSColor(white: 0, alpha: 0.9)
  sh.shadowBlurRadius = 10
  sh.shadowOffset = NSSize(width: 0, height: -3)
  attrs[.shadow] = sh
  if stroke {  // 太い縁取り（小さく表示されても読めるように）
    var o = attrs
    o[.strokeColor] = NSColor.black
    o[.strokeWidth] = 14
    o[.foregroundColor] = NSColor.black
    let a = NSAttributedString(string: s, attributes: o)
    let px = center ? x - a.size().width / 2 : x
    a.draw(at: NSPoint(x: px, y: H - y - a.size().height))
  }
  let a = NSAttributedString(string: s, attributes: attrs)
  let sz = a.size()
  let px = center ? x - sz.width / 2 : x
  a.draw(at: NSPoint(x: px, y: H - y - sz.height))
  return sz
}

func rect(_ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat, _ c: NSColor, radius: CGFloat = 0) {
  c.setFill()
  NSBezierPath(roundedRect: NSRect(x: x, y: H - y - h, width: w, height: h), xRadius: radius, yRadius: radius).fill()
}

let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: Int(W), pixelsHigh: Int(H), bitsPerSample: 8, samplesPerPixel: 4,
                           hasAlpha: true, isPlanar: false, colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0)!
NSGraphicsContext.saveGraphicsState()
NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
NSGraphicsContext.current!.imageInterpolation = .high
rect(0, 0, W, H, bg)

// 右: 写真の真ん中あたり（2 つの基板）を、右 600px に縦いっぱいで置く
let ps = photo.size
let cropW = ps.height * 600 / (H - 110)  // 下の帯（110px）の上に収める
let src = NSRect(x: (ps.width - cropW) / 2, y: 0, width: cropW, height: ps.height)
photo.draw(in: NSRect(x: W - 600, y: 110, width: 600, height: H - 110), from: src, operation: .copy, fraction: 1)
// 写真の左端をなじませる
let fade = NSGradient(starting: bg, ending: bg.withAlphaComponent(0))!
fade.draw(in: NSRect(x: W - 600, y: 110, width: 180, height: H - 110), angle: 0)

// 左: 文字
text("Raspberry Pi Pico 2 W で", font(8, 40), amber, x: 56, y: 52, stroke: true)
text("オールド", font(9, 138), ink, x: 48, y: 104, kern: 2, stroke: true)
text("シンセ", font(9, 138), ink, x: 48, y: 252, kern: 2, stroke: true)
text("を作ろう", font(9, 96), amber, x: 56, y: 400, stroke: true)
text("DIY Vintage Synth", font(8, 44), ink, x: 60, y: 520, stroke: true)

// 右上: 目印
rect(W - 290, 36, 250, 92, NSColor(red: 0.85, green: 0.20, blue: 0.20, alpha: 1), radius: 14)
text("部品 2 つ", font(9, 44), .white, x: W - 165, y: 44, center: true)
text("2 parts only", font(7, 22), .white, x: W - 165, y: 96, center: true)

// 下: 7 機種（操作画面のそれぞれの色）
let models: [(String, NSColor, NSColor)] = [
  ("MINI", NSColor(red: 0.11, green: 0.11, blue: 0.11, alpha: 1), NSColor(red: 0.78, green: 0.63, blue: 0.38, alpha: 1)),
  ("ACID", NSColor(red: 0.79, green: 0.79, blue: 0.77, alpha: 1), NSColor(red: 0.91, green: 0.45, blue: 0.16, alpha: 1)),
  ("JUNO", NSColor(red: 0.11, green: 0.11, blue: 0.12, alpha: 1), NSColor(red: 0.88, green: 0.19, blue: 0.23, alpha: 1)),
  ("PRO-5", NSColor(red: 0.09, green: 0.08, blue: 0.08, alpha: 1), NSColor(red: 0.85, green: 0.79, blue: 0.63, alpha: 1)),
  ("MS", NSColor(red: 0.16, green: 0.17, blue: 0.18, alpha: 1), NSColor.white),
  ("SEM", NSColor(red: 0.91, green: 0.89, blue: 0.81, alpha: 1), NSColor(red: 0.16, green: 0.30, blue: 0.56, alpha: 1)),
  ("STRINGS", NSColor(red: 0.23, green: 0.14, blue: 0.09, alpha: 1), NSColor(red: 0.88, green: 0.69, blue: 0.31, alpha: 1)),
]
let gap: CGFloat = 8, x0: CGFloat = 24, y0: CGFloat = H - 96, bh: CGFloat = 76
let bw = (W - x0 * 2 - gap * CGFloat(models.count - 1)) / CGFloat(models.count)
for (i, m) in models.enumerated() {
  let x = x0 + CGFloat(i) * (bw + gap)
  rect(x, y0, bw, bh, m.1, radius: 8)
  rect(x, y0, bw, 6, m.2, radius: 3)
  let f = font(9, m.0.count > 5 ? 28 : 34)
  let a = NSAttributedString(string: m.0, attributes: [.font: f, .foregroundColor: m.2, .kern: 1])
  a.draw(at: NSPoint(x: x + (bw - a.size().width) / 2, y: H - y0 - bh / 2 - a.size().height / 2 - 2))
}
text("7 機種を切り替え / 7 models", font(8, 26), ink, x: 60, y: y0 - 50, stroke: true)

NSGraphicsContext.restoreGraphicsState()
try! rep.representation(using: .png, properties: [:])!.write(to: URL(fileURLWithPath: args[2]))
print("wrote", args[2])
