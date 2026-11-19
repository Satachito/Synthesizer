// 自動デモ。機種ごとに短いフレーズを、今の音色のまま繰り返す（フレーズはこのプロジェクトのための書き下ろし）
// 音の側のコア（Core1）で、render() の直前に毎ブロック tick() を呼ぶ。タイミングは 1 ブロック（約 0.33ms）単位
#pragma once
#include "synth_engine.h"

struct DemoNote {
  uint8_t step;   // 16 分音符で何個目から
  uint8_t len;    // 16 分音符いくつぶん
  uint8_t note;   // MIDI のノート番号
  uint8_t vel;    // ACID は 100 以上でアクセント
  uint8_t slide;  // 1 = 次の音と重ねる（モノの機種ではスライド / グライドになる）
};

struct DemoPattern {
  uint16_t bpm;
  uint8_t steps;  // 1 周の長さ（16 分音符の数）
  uint8_t count;
  const DemoNote *notes;  // step の順に並べる
};

// 和音は同じ step に並べる。const にしないのは RAM に置くため（フラッシュはもう片方のコアとキャッシュを取り合う）
#define DN_CHORD4(st, ln, a, b, c, d, v) { st, ln, a, v, 0 }, { st, ln, b, v, 0 }, { st, ln, c, v, 0 }, { st, ln, d, v, 0 }

// MINI: A マイナーのベースからリードへ。最後の 2 音はグライドでつなぐ
static DemoNote DEMO_MINI[] = {
  { 0, 2, 57, 110, 0 }, { 2, 2, 57, 80, 0 }, { 4, 2, 60, 100, 0 }, { 6, 2, 57, 80, 0 },
  { 8, 3, 64, 110, 0 }, { 11, 1, 62, 80, 0 }, { 12, 4, 60, 100, 0 },
  { 16, 2, 55, 110, 0 }, { 18, 2, 55, 80, 0 }, { 20, 2, 59, 100, 0 }, { 22, 2, 55, 80, 0 },
  { 24, 2, 62, 110, 0 }, { 26, 2, 64, 100, 1 }, { 28, 4, 67, 110, 0 },
};
// ACID: C マイナーの 16 ステップ。強い音がアクセント、slide = 1 がスライド
static DemoNote DEMO_ACID[] = {
  { 0, 1, 48, 120, 0 }, { 2, 1, 48, 70, 0 }, { 3, 1, 60, 70, 1 }, { 4, 1, 48, 120, 0 },
  { 6, 1, 51, 70, 0 }, { 7, 1, 48, 120, 0 }, { 8, 1, 55, 70, 1 }, { 9, 1, 58, 110, 0 },
  { 10, 1, 48, 70, 0 }, { 11, 1, 60, 120, 0 }, { 12, 1, 48, 70, 0 }, { 13, 1, 51, 70, 1 },
  { 14, 1, 53, 120, 0 }, { 15, 1, 48, 70, 0 },
};
// JUNO: Am7 と Fmaj7 を刻む
static DemoNote DEMO_JUNO[] = {
  DN_CHORD4(0, 5, 57, 60, 64, 67, 95), DN_CHORD4(6, 2, 57, 60, 64, 67, 80), DN_CHORD4(8, 7, 57, 60, 64, 67, 90),
  DN_CHORD4(16, 5, 53, 57, 60, 64, 95), DN_CHORD4(22, 2, 53, 57, 60, 64, 80), DN_CHORD4(24, 7, 55, 59, 62, 67, 90),
};
// PRO-5: ブラスの刻み（C から B♭）
static DemoNote DEMO_PRO5[] = {
  DN_CHORD4(0, 2, 48, 55, 60, 64, 110), DN_CHORD4(3, 2, 48, 55, 60, 64, 90), DN_CHORD4(6, 9, 48, 55, 60, 64, 105),
  DN_CHORD4(16, 2, 46, 53, 58, 62, 110), DN_CHORD4(19, 2, 46, 53, 58, 62, 90), DN_CHORD4(22, 9, 46, 53, 58, 62, 105),
};
// MS: E のリフ
static DemoNote DEMO_MS[] = {
  { 0, 1, 52, 120, 0 }, { 2, 1, 52, 80, 0 }, { 3, 1, 55, 90, 0 }, { 4, 1, 52, 110, 0 },
  { 6, 1, 57, 90, 0 }, { 7, 1, 52, 80, 0 }, { 8, 2, 59, 115, 0 }, { 10, 1, 62, 90, 0 },
  { 11, 1, 64, 110, 0 }, { 12, 1, 52, 80, 0 }, { 14, 2, 55, 100, 0 },
};
// SEM: Dm7 と G7sus4 をゆったり
static DemoNote DEMO_SEM[] = {
  DN_CHORD4(0, 14, 50, 57, 60, 65, 90),
  DN_CHORD4(16, 14, 43, 55, 60, 65, 90),
};
// STRINGS: C と Am をつなぐ（8 音まで鳴る）
static DemoNote DEMO_STR[] = {
  DN_CHORD4(0, 15, 48, 55, 64, 67, 90), { 0, 15, 72, 80, 0 },
  DN_CHORD4(16, 15, 45, 52, 60, 64, 90), { 16, 15, 69, 80, 0 },
};

#define DEMO_PAT(bpm, steps, arr) { bpm, steps, (uint8_t)(sizeof(arr) / sizeof(arr[0])), arr }
// MODELS と同じ順番
static DemoPattern DEMO_PATTERNS[] = {
  DEMO_PAT(110, 32, DEMO_MINI), DEMO_PAT(128, 16, DEMO_ACID), DEMO_PAT(100, 32, DEMO_JUNO),
  DEMO_PAT(96, 32, DEMO_PRO5), DEMO_PAT(120, 16, DEMO_MS), DEMO_PAT(80, 32, DEMO_SEM),
  DEMO_PAT(66, 32, DEMO_STR),
};
static_assert(sizeof(DEMO_PATTERNS) / sizeof(DEMO_PATTERNS[0]) == sizeof(MODELS) / sizeof(MODELS[0]), "機種ごとに 1 つ");

class DemoPlayer {
public:
  volatile bool on = false;    // Core0 が書く
  volatile bool tour = false;  // Core0 が書く。true = 1 フレーズ弾いたら止まって phrasesDone を増やす（Core0 が次の機種へ切り替える）
  volatile uint32_t phrasesDone = 0;  // Core1 が書く

  // Core1: 毎ブロック、synth.render() の直前に呼ぶ
  SYNTH_RAM void tick(Synth &s) {
    if (!on) {
      if (running) stop(s);
      return;
    }
    if (s.switching()) return;  // 機種を切り替えている途中は待つ
    int m = s.currentModel();
    if (!running || m != pm) {
      stop(s);
      start(m);
    }
    const DemoPattern &p = DEMO_PATTERNS[pm];
    // 離す
    for (int i = 0; i < nAct;) {
      if (--act[i].left <= 0) {
        s.eventNow(Synth::EV_OFF, act[i].note);
        act[i] = act[--nAct];
      } else {
        i++;
      }
    }
    if (holding) return;  // 全機種デモ: 弾き終わって、Core0 が次の機種に切り替えるのを待っている
    // 押す
    while (idx < p.count && (float)p.notes[idx].step * stepBlocks <= t) {
      const DemoNote &n = p.notes[idx++];
      float len = (float)n.len * stepBlocks;
      // ふつうは音の長さの 6 割で離す（歯切れよく）。slide は次の音まで重ねる
      int left = (int)(n.slide ? len + stepBlocks * 0.3f : len * 0.6f);
      if (left < 1) left = 1;
      s.eventNow(Synth::EV_ON, n.note, n.vel);
      if (nAct < MAXACT) act[nAct++] = { n.note, left };
    }
    t += 1.0f;
    if (t >= (float)p.steps * stepBlocks) {  // 1 周したら頭へ
      t -= (float)p.steps * stepBlocks;
      idx = 0;
      if (tour && ++loops >= loopsPerPhrase) {
        holding = true;
        phrasesDone = phrasesDone + 1;
      }
    }
  }

private:
  void start(int m) {
    pm = m;
    running = true;
    // 全機種デモ: 画面が次の機種に切り替わるのを待ってから弾き始める（1 秒）
    t = tour ? -TOUR_LEAD_SEC * Synth::SR / (float)Synth::BLOCK : 0.0f;
    idx = 0;
    stepBlocks = 60.0f / (float)DEMO_PATTERNS[m].bpm / 4.0f * Synth::SR / (float)Synth::BLOCK;
    // 全機種デモで 1 機種あたりの長さをそろえる: 3 秒に満たない短いフレーズは 2 回弾く
    float sec = (float)DEMO_PATTERNS[m].steps * 15.0f / (float)DEMO_PATTERNS[m].bpm;
    loopsPerPhrase = sec < 3.0f ? 2 : 1;
    loops = 0;
    holding = false;
  }
  SYNTH_RAM void stop(Synth &s) {
    if (running) s.eventNow(Synth::EV_ALLOFF);
    running = false;
    holding = false;
    nAct = 0;
  }

  struct Active {
    uint8_t note;
    int left;  // あと何ブロックで離すか
  };
  static constexpr int MAXACT = 16;
  static constexpr float TOUR_LEAD_SEC = 1.0f;
  Active act[MAXACT];
  int nAct = 0, pm = 0, idx = 0;
  bool running = false, holding = false;
  int loops = 0, loopsPerPhrase = 1;
  float t = 0.0f, stepBlocks = 1.0f;
};
