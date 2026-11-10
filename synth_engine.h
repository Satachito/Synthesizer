// シンセのエンジン。Arduino に依存しないので PC でもそのままコンパイルできる（test/host_test.cpp）
//   オシレーター x3（PolyBLEP）+ ノイズ + リング → HPF → フィルター（ラダー / SVF / MS）→ VCA → ディレイ → コーラス
// render() を音の側のコア（Core1）が呼び、P[] の書き換えと post() はもう片方のコア（Core0）が呼ぶ
#pragma once
#include <math.h>
#include <string.h>
#include <stdint.h>
#include "synth_params.h"

// 音の計算は RAM に置く。フラッシュ上だと、もう片方のコアの Wi-Fi 処理がフラッシュのキャッシュ（16KB）を
// 使い回すたびに読み直しになって、数倍遅くなる
#ifdef ARDUINO
#define SYNTH_RAM __attribute__((noinline, section(".time_critical.synth")))
#else
#define SYNTH_RAM
#endif
#define SYNTH_INL inline __attribute__((always_inline))

class Synth {
public:
  static constexpr float SR = 48000.0f;
  static constexpr int BLOCK = 16;  // この単位で EG・LFO・音程を更新する（3kHz）
  static constexpr int MAXV = 8;

  float P[NP];                 // 今の音色。Core0 が書き、書いたら gen を増やす
  volatile uint32_t gen = 1;
  volatile int model = 0;      // Core0 が書く。render() が次のブロックで切り替える
  volatile int activeVoices = 0;

  enum EvType : uint8_t { EV_ON, EV_OFF, EV_BEND, EV_MODW, EV_SUS, EV_ALLOFF };

  // Core0 から呼ぶ。EV_BEND は a = 下位 7bit, b = 上位 7bit
  bool post(uint8_t type, uint8_t a = 0, uint8_t b = 0) {
    uint32_t h = qHead, nx = (h + 1) & (QN - 1);
    if (nx == qTail) return false;
    q[h] = (uint32_t)type | ((uint32_t)a << 8) | ((uint32_t)b << 16);
    qHead = nx;
    return true;
  }

  void begin() {
    for (int i = 0; i <= 256; i++) pow2tab[i] = powf(2.0f, i / 256.0f);
    modelDefaults(0, P);
    cur = -1;
    cutS = P[P_CUT];
  }

  // out: ステレオ 16bit、BLOCK フレーム（L, R, L, R, ...）
  SYNTH_RAM void render(int16_t *out) {
    if (model != cur) switchModel(model);
    if (gen != lastGen) {
      lastGen = gen;
      derive();
    }
    while (qTail != qHead) {
      uint32_t e = q[qTail];
      qTail = (qTail + 1) & (QN - 1);
      handle(e & 0xff, (e >> 8) & 0xff, (e >> 16) & 0xff);
    }

    // ブロックごとの共通の値
    cutS += (P[P_CUT] - cutS) * 0.08f;
    volS += (d.vol - volS) * 0.08f;
    bendS += (bend - bendS) * 0.3f;
    lfoPh += d.lfoInc;
    if (lfoPh >= 1.0f) {
      lfoPh -= 1.0f;
      lfoSH = rnd();
    }
    switch (d.lwave) {
      case 0: lfoVal = 4.0f * fabsf(lfoPh - 0.5f) - 1.0f; break;
      case 1: lfoVal = 1.0f - 2.0f * lfoPh; break;
      case 2: lfoVal = lfoPh < 0.5f ? 1.0f : -1.0f; break;
      default: lfoVal = lfoSH; break;
    }
    pitchMod = bendS * 2.0f + lfoVal * (d.vib + modw * 0.6f);

    float mix[BLOCK];
    for (int i = 0; i < BLOCK; i++) mix[i] = 0.0f;
    int n = 0;
    for (int i = 0; i < nv; i++) {
      if (!V[i].active) continue;
      renderVoice(V[i], mix);
      n++;
    }
    activeVoices = n;

    // ディレイ（モノ）
    if (d.dmix > 0.001f) {
      if (!delayOn) {
        memset(dbuf, 0, sizeof(dbuf));
        delayOn = true;
      }
      for (int i = 0; i < BLOCK; i++) {
        dlenS += (d.dlen - dlenS) * 0.0004f;
        float rp = (float)dw - dlenS;
        if (rp < 0.0f) rp += (float)DN;
        int i0 = (int)rp;
        float fr = rp - (float)i0;
        float a = dbuf[i0 & (DN - 1)], b = dbuf[(i0 + 1) & (DN - 1)];
        float y = (a + (b - a) * fr) * (1.0f / 16384.0f);
        dlp += (y - dlp) * 0.4f;  // 繰り返すたびに高域が減る
        float w = (mix[i] + dlp * d.dfb) * 16384.0f;
        if (w > 32767.0f) w = 32767.0f;
        else if (w < -32767.0f) w = -32767.0f;
        dbuf[dw] = (int16_t)w;
        dw = (dw + 1) & (DN - 1);
        mix[i] += y * d.dmix;
      }
    } else {
      delayOn = false;
    }

    // コーラス（ここでステレオになる）と出力
    for (int i = 0; i < BLOCK; i++) {
      float x = mix[i], l, r;
      if (d.cho == 0) {
        l = r = x;
      } else {
        cbuf[cw] = x;
        if (d.cho < 3) {  // Juno 風: 三角波の LFO で左右を逆に揺らす
          choPh += d.choInc;
          if (choPh >= 1.0f) choPh -= 1.0f;
          float t = 4.0f * fabsf(choPh - 0.5f) - 1.0f;
          float wl = tap(168.0f + 86.0f * t), wr = tap(168.0f - 86.0f * t);
          l = (x + wl) * 0.75f;
          r = (x + wr) * 0.75f;
        } else {  // アンサンブル: 120 度ずつずれた 3 本を、遅い揺れと速い揺れで動かす
          choPh += 0.6f / SR;
          if (choPh >= 1.0f) choPh -= 1.0f;
          choPh2 += 6.1f / SR;
          if (choPh2 >= 1.0f) choPh2 -= 1.0f;
          float t0 = tap(240.0f + 110.0f * fsin(choPh) + 14.0f * fsin(choPh2));
          float t1 = tap(240.0f + 110.0f * fsin(frac1(choPh + 0.3333f)) + 14.0f * fsin(frac1(choPh2 + 0.3333f)));
          float t2 = tap(240.0f + 110.0f * fsin(frac1(choPh + 0.6667f)) + 14.0f * fsin(frac1(choPh2 + 0.6667f)));
          l = (x * 0.3f + t0 + t1 * 0.5f) * 0.65f;
          r = (x * 0.3f + t2 + t1 * 0.5f) * 0.65f;
        }
        cw = (cw + 1) & (CN - 1);
      }
      out[i * 2] = (int16_t)(softclip(l * volS) * 32000.0f);
      out[i * 2 + 1] = (int16_t)(softclip(r * volS) * 32000.0f);
    }
  }

private:
  // ---- 小物 ----
  float pow2tab[257];
  SYNTH_INL float fpow2(float x) const {  // 2^x。表引き + 直線補間
    if (x > 20.0f) x = 20.0f;
    else if (x < -20.0f) x = -20.0f;
    float fl = floorf(x);
    int i = (int)fl;
    float fr = (x - fl) * 256.0f;
    int k = (int)fr;
    float a = pow2tab[k];
    float v = a + (pow2tab[k + 1] - a) * (fr - (float)k);
    return i >= 0 ? v * (float)(1u << i) : v / (float)(1u << (-i));
  }
  static SYNTH_INL float softclip(float x) {  // ±1.5 で ±1 に張りつく
    if (x > 1.5f) x = 1.5f;
    else if (x < -1.5f) x = -1.5f;
    return x - x * x * x * (4.0f / 27.0f);
  }
  static SYNTH_INL float sat2(float x) { return softclip(x * 0.5f) * 2.0f; }
  static SYNTH_INL float frac1(float x) { return x >= 1.0f ? x - 1.0f : x; }
  static SYNTH_INL float fsin(float ph) {  // ph: 0..1
    float x = 2.0f * ph - 1.0f;
    float y = 4.0f * x * (1.0f - fabsf(x));
    return -(0.225f * (y * fabsf(y) - y) + y);
  }
  static SYNTH_INL float tang(float fc) {  // tan(π fc / SR) の近似（fc は 17kHz まで）
    float x = fc * (3.14159265f / SR);
    float x2 = x * x;
    return x * (15.0f - x2) / (15.0f - 6.0f * x2);
  }
  uint32_t seed = 0x1234567;
  SYNTH_INL float rnd() {  // -1..1
    seed = seed * 1664525u + 1013904223u;
    return (float)(int32_t)seed * (1.0f / 2147483648.0f);
  }
  static SYNTH_INL float blep(float t, float dt) {
    if (t < dt) {
      t /= dt;
      return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
      t = (t - 1.0f) / dt;
      return t * t + t + t + 1.0f;
    }
    return 0.0f;
  }

  // ---- オシレーター ----
  template<int W> static SYNTH_INL void oscT(float &phase, float inc, float pw, float *out, const float *fm, float fmAmt) {
    float ph = phase;
    for (int i = 0; i < BLOCK; i++) {
      float dt = inc;
      if (fm) {
        dt = inc * (1.0f + fmAmt * fm[i]);
        if (dt < 1e-6f) dt = 1e-6f;
        else if (dt > 0.45f) dt = 0.45f;
      }
      ph += dt;
      if (ph >= 1.0f) ph -= 1.0f;
      float y;
      if (W == 0) {
        y = 2.0f * ph - 1.0f - blep(ph, dt);
      } else if (W == 1) {
        float t2 = ph + 1.0f - pw;
        if (t2 >= 1.0f) t2 -= 1.0f;
        y = (ph < pw ? 1.0f : -1.0f) + blep(ph, dt) - blep(t2, dt) - (2.0f * pw - 1.0f);
      } else if (W == 2) {
        y = 1.0f - 4.0f * fabsf(ph - 0.5f);
      } else {
        y = fsin(ph);
      }
      out[i] = y;
    }
    phase = ph;
  }
  SYNTH_RAM static void osc(int wave, float &phase, float inc, float pw, float *out, const float *fm, float fmAmt) {
    switch (wave) {
      case 0: oscT<0>(phase, inc, pw, out, fm, fmAmt); break;
      case 1: oscT<1>(phase, inc, pw, out, fm, fmAmt); break;
      case 2: oscT<2>(phase, inc, pw, out, fm, fmAmt); break;
      default: oscT<3>(phase, inc, pw, out, fm, fmAmt); break;
    }
  }

  // ---- エンベロープ ----
  struct Env {
    float v = 0;
    uint8_t st = 0;  // 0 止まっている 1 アタック 2 ディケイ〜サステイン 4 リリース
  };
  static SYNTH_INL float envStep(Env &e, float aC, float dC, float s, float rC) {
    switch (e.st) {
      case 1:
        e.v += (1.3f - e.v) * aC;
        if (e.v >= 1.0f) {
          e.v = 1.0f;
          e.st = 2;
        }
        break;
      case 2: e.v += (s - e.v) * dC; break;
      case 4:
        e.v -= e.v * rC;
        if (e.v < 1e-4f) {
          e.v = 0.0f;
          e.st = 0;
        }
        break;
    }
    return e.v;
  }

  struct Voice {
    bool active = false, gate = false, held = false;
    uint8_t note = 60;
    float vel = 1, accent = 0;
    float pitch = 60, target = 60;
    float ph1 = 0, ph2 = 0.37f, ph3 = 0.71f;
    float s1 = 0, s2 = 0, s3 = 0, s4 = 0;  // ラダー
    float ic1 = 0, ic2 = 0;                // SVF
    float h1 = 0, h2 = 0;                  // HPF
    float gPrev = -1, ghPrev = -1, ampPrev = 0;
    Env fe, ae;
    uint32_t age = 0;
  };

  // P[] から計算しておく値
  struct Derived {
    int w1, w2, w3, lwave, cho;
    float l1, l2, l3, noise, ring, fm, pmenv;
    float oct1, oct2, oct3, pw, pwm;
    float k, kh, fenv, fkbd, flfo, fmode, hpC, gh;
    float faC, fdC, fs, frC, aaC, adC, as, arC;
    float vels, acc, lfoInc, vib, glideC, post, vol;
    float choInc, dlen, dfb, dmix;
    float comp;
    int filt;
    bool legato;
  } d;

  static float envTime(float x) { return 0.001f * powf(10.0f, 4.0f * x); }  // 1ms .. 10s
  static float envCoef(float x, float div) {
    float c = 1.0f - expf(-(float)BLOCK * div / (SR * envTime(x)));
    return c;
  }

  void derive() {
    const ModelDef &md = MODELS[cur];
    d.filt = md.filt;
    d.comp = md.comp;
    float dr = P[P_DRIVE];
    float pre = 0.25f + 1.75f * dr * dr;
    d.w1 = (int)P[P_O1WAVE];
    d.w2 = (int)P[P_O2WAVE];
    d.w3 = (int)P[P_O3WAVE];
    d.l1 = P[P_O1LVL] * pre;
    d.l2 = P[P_O2LVL] * pre;
    d.l3 = P[P_O3LVL] * pre;
    d.noise = P[P_NOISE] * pre;
    d.ring = P[P_RING] * pre;
    d.fm = P[P_PMOSC] * P[P_PMOSC] * 4.0f;
    d.pmenv = P[P_PMENV] * P[P_PMENV] * 2.0f;
    d.oct1 = (float)((int)P[P_O1OCT] - 2);
    d.oct2 = (float)((int)P[P_O2OCT] - 2) + ((float)((int)P[P_O2SEMI] - 12) + (P[P_O2DET] - 0.5f)) * (1.0f / 12.0f);
    d.oct3 = (float)((int)P[P_O3OCT] - 2) + (P[P_O3DET] - 0.5f) * (1.0f / 12.0f);
    d.pw = 0.5f + 0.45f * P[P_PW];
    d.pwm = 0.4f * P[P_PWM];
    if (md.filt == FILT_LADDER) d.k = P[P_RES] * md.resMax;
    else if (md.filt == FILT_MS) d.k = 2.0f - 2.12f * P[P_RES];  // 少しマイナスまで行って発振する
    else d.k = 2.0f - 1.95f * P[P_RES];
    d.kh = 2.0f - 2.1f * P[P_HPRES];
    d.fenv = P[P_FENV] * 10.0f;
    d.fkbd = P[P_FKBD];
    d.flfo = P[P_FLFO] * 4.0f;
    d.fmode = P[P_FMODE];
    d.gh = tang(20.0f * powf(2.0f, P[P_HPF] * 8.0f));
    d.hpC = P[P_HPF] < 0.01f ? 0.0f : 1.0f - expf(-6.2832f * 10.0f * powf(2.0f, P[P_HPF] * 8.0f) / SR);
    d.faC = envCoef(P[P_FA], 1.5f);
    d.fdC = envCoef(P[P_FD], 3.0f);
    d.fs = P[P_FS];
    d.frC = envCoef(P[P_FR], 3.0f);
    d.aaC = envCoef(P[P_AA], 1.5f);
    d.adC = envCoef(P[P_AD], 3.0f);
    d.as = P[P_AS];
    d.arC = envCoef(P[P_AR], 3.0f);
    d.vels = P[P_VELS];
    d.acc = P[P_ACC];
    d.lwave = (int)P[P_LWAVE];
    d.lfoInc = 0.05f * powf(2.0f, P[P_LRATE] * 9.2f) * (float)BLOCK / SR;  // 0.05 .. 30Hz
    d.vib = P[P_LPITCH] * P[P_LPITCH] * 7.0f;
    d.glideC = P[P_GLIDE] < 0.01f ? 1.0f : 1.0f - expf(-(float)BLOCK * 3.0f / (SR * 0.005f * powf(400.0f, P[P_GLIDE])));
    d.legato = P[P_LEGATO] >= 0.5f;
    d.post = (md.voices == 1 ? 0.8f : 0.42f) / sqrtf(pre);
    d.vol = P[P_VOL] * P[P_VOL] * 1.4f;
    d.cho = (int)P[P_CHO];
    d.choInc = (d.cho == 2 ? 0.83f : 0.5f) / SR;
    d.dlen = (0.03f + 0.62f * P[P_DTIME]) * SR;
    d.dfb = P[P_DFB] * 0.9f;
    d.dmix = P[P_DMIX];
  }

  // ---- 1 音ぶん ----
  SYNTH_RAM void renderVoice(Voice &v, float *mix) {
    const float inv = 1.0f / (float)BLOCK;
    if (d.glideC >= 1.0f) v.pitch = v.target;
    else v.pitch += (v.target - v.pitch) * d.glideC;
    float fe = envStep(v.fe, d.faC, d.fdC, d.fs, d.frC);
    float ae = envStep(v.ae, d.aaC, d.adC, d.as, d.arC);
    if (v.ae.st == 0) v.active = false;  // このブロックで音量を 0 まで下げて終わる

    float n = v.pitch + pitchMod;
    float b = (n - 69.0f) * (1.0f / 12.0f);
    const float K440 = 440.0f / SR;
    float inc1 = K440 * fpow2(b + d.oct1 + d.pmenv * fe);
    float inc2 = K440 * fpow2(b + d.oct2);
    float inc3 = K440 * fpow2(b + d.oct3);
    if (inc1 > 0.45f) inc1 = 0.45f;
    if (inc2 > 0.45f) inc2 = 0.45f;
    if (inc3 > 0.45f) inc3 = 0.45f;
    float pw = d.pw + d.pwm * lfoVal;
    if (pw > 0.95f) pw = 0.95f;
    else if (pw < 0.05f) pw = 0.05f;

    float b1[BLOCK], b2[BLOCK], mx[BLOCK];
    bool need2 = d.l2 > 0.0f || d.ring > 0.0f || d.fm > 0.0f;
    if (need2) osc(d.w2, v.ph2, inc2, pw, b2, nullptr, 0.0f);
    osc(d.w1, v.ph1, inc1, pw, b1, d.fm > 0.0f ? b2 : nullptr, d.fm);
    for (int i = 0; i < BLOCK; i++) mx[i] = b1[i] * d.l1;
    if (need2)
      for (int i = 0; i < BLOCK; i++) mx[i] += b2[i] * (d.l2 + b1[i] * d.ring);
    if (d.l3 > 0.0f) {
      osc(d.w3, v.ph3, inc3, pw, b2, nullptr, 0.0f);
      for (int i = 0; i < BLOCK; i++) mx[i] += b2[i] * d.l3;
    }
    if (d.noise > 0.0f)
      for (int i = 0; i < BLOCK; i++) mx[i] += rnd() * d.noise;

    // カットオフ（オクターブ単位で足し合わせる）
    float acc = 1.0f + v.accent;
    float oct = cutS * 10.0f + d.fenv * fe * acc + d.fkbd * (n - 60.0f) * (1.0f / 12.0f) + d.flfo * lfoVal;
    float fc = 20.0f * fpow2(oct);
    if (fc > 17000.0f) fc = 17000.0f;
    else if (fc < 20.0f) fc = 20.0f;
    float g1 = tang(fc);

    if (d.filt == FILT_LADDER) {
      if (d.hpC > 0.0f)
        for (int i = 0; i < BLOCK; i++) {
          v.h1 += (mx[i] - v.h1) * d.hpC;
          mx[i] -= v.h1;
        }
      // 4 段の 1 次ローパスを遅れなしのフィードバックで解く。歪みは入力の足し算のところに 1 か所
      float G1 = g1 / (1.0f + g1);
      float G = v.gPrev < 0.0f ? G1 : v.gPrev, dG = (G1 - G) * inv;
      v.gPrev = G1;
      float k = d.k, ing = 1.0f + d.comp * k;
      float s1 = v.s1, s2 = v.s2, s3 = v.s3, s4 = v.s4;
      for (int i = 0; i < BLOCK; i++) {
        G += dG;
        float G2 = G * G;
        float S = (1.0f - G) * (((s1 * G + s2) * G + s3) * G + s4);
        float x = softclip((mx[i] * ing - k * S) / (1.0f + k * G2 * G2));
        float t;
        t = (x - s1) * G; x = t + s1; s1 = x + t;
        t = (x - s2) * G; x = t + s2; s2 = x + t;
        t = (x - s3) * G; x = t + s3; s3 = x + t;
        t = (x - s4) * G; x = t + s4; s4 = x + t;
        mx[i] = x;
      }
      v.s1 = s1; v.s2 = s2; v.s3 = s3; v.s4 = s4;
    } else {
      bool ms = d.filt == FILT_MS;
      if (ms) {  // 発振する 12dB の HPF
        float g = v.ghPrev < 0.0f ? d.gh : v.ghPrev, dg = (d.gh - g) * inv;
        v.ghPrev = d.gh;
        float k = d.kh, ic1 = v.h1, ic2 = v.h2;
        for (int i = 0; i < BLOCK; i++) {
          g += dg;
          float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
          float in = mx[i];
          float v3 = in - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
          ic1 = sat2(2.0f * v1 - ic1);
          ic2 = sat2(2.0f * v2 - ic2);
          mx[i] = in - k * v1 - v2;
        }
        v.h1 = ic1; v.h2 = ic2;
      } else if (d.hpC > 0.0f) {
        for (int i = 0; i < BLOCK; i++) {
          v.h1 += (mx[i] - v.h1) * d.hpC;
          mx[i] -= v.h1;
        }
      }
      float g = v.gPrev < 0.0f ? g1 : v.gPrev, dg = (g1 - g) * inv;
      v.gPrev = g1;
      float k = d.k, m = ms ? 0.0f : d.fmode, ic1 = v.ic1, ic2 = v.ic2;
      for (int i = 0; i < BLOCK; i++) {
        g += dg;
        float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        float in = softclip(mx[i]);
        float v3 = in - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        if (ms) {
          ic1 = sat2(ic1);
          ic2 = sat2(ic2);
        } else {
          if (ic1 > 4.0f) ic1 = 4.0f; else if (ic1 < -4.0f) ic1 = -4.0f;
          if (ic2 > 4.0f) ic2 = 4.0f; else if (ic2 < -4.0f) ic2 = -4.0f;
        }
        float hp = in - k * v1 - v2;
        mx[i] = v2 + m * (hp - v2);
      }
      v.ic1 = ic1; v.ic2 = ic2;
    }

    float amp = ae * (1.0f - d.vels + d.vels * v.vel) * (1.0f + 0.6f * v.accent) * d.post;
    float a = v.ampPrev, da = (amp - a) * inv;
    v.ampPrev = amp;
    for (int i = 0; i < BLOCK; i++) {
      a += da;
      mix[i] += mx[i] * a;
    }
  }

  // ---- 鍵盤 ----
  void startVoice(Voice &v, int note, float vel, bool glideFromPrev) {
    const ModelDef &md = MODELS[cur];
    if (!v.active) {
      v.fe.v = v.ae.v = 0.0f;
      v.s1 = v.s2 = v.s3 = v.s4 = v.ic1 = v.ic2 = v.h1 = v.h2 = 0.0f;
      v.gPrev = v.ghPrev = -1.0f;
      v.ampPrev = 0.0f;
      if (md.flags & MF_DCO) v.ph1 = v.ph2 = v.ph3 = 0.0f;
    }
    v.note = note;
    v.target = (float)note;
    if (!glideFromPrev) v.pitch = v.target;
    v.vel = vel;
    v.accent = ((md.flags & MF_ACID) && vel >= 0.78f) ? d.acc : 0.0f;
    v.gate = true;
    v.held = false;
    v.fe.st = v.ae.st = 1;
    v.active = true;
    v.age = ++clock;
  }
  static void release(Voice &v) {
    v.gate = false;
    v.held = false;
    if (v.fe.st) v.fe.st = 4;
    if (v.ae.st) v.ae.st = 4;
  }
  void stackRemove(int note) {
    for (int i = 0; i < nStack; i++)
      if (stack[i] == note) {
        for (int j = i; j < nStack - 1; j++) stack[j] = stack[j + 1];
        nStack--;
        return;
      }
  }

  void noteOn(int note, float vel) {
    if (nv == 1) {  // モノ: 後から押した鍵が優先
      Voice &v = V[0];
      bool overlap = v.active && v.gate && nStack > 0;
      stackRemove(note);
      if (nStack < 16) stack[nStack++] = note;
      if (d.legato && overlap) {  // スライド。EG は掛け直さない
        v.note = note;
        v.target = (float)note;
        if (MODELS[cur].flags & MF_ACID) v.accent = vel >= 0.78f ? d.acc : 0.0f;
      } else {
        startVoice(v, note, vel, !d.legato && everPlayed);
      }
      everPlayed = true;
      return;
    }
    int vi = -1;
    for (int i = 0; i < nv; i++)
      if (V[i].active && V[i].note == note) { vi = i; break; }
    if (vi < 0)
      for (int i = 0; i < nv; i++)
        if (!V[i].active) { vi = i; break; }
    if (vi < 0) {  // 空きがない: 離した音のうち古いもの、なければいちばん古い音を奪う
      uint32_t best = 0xffffffffu;
      for (int pass = 0; pass < 2 && vi < 0; pass++)
        for (int i = 0; i < nv; i++) {
          if (pass == 0 && V[i].gate) continue;
          if (V[i].age < best) { best = V[i].age; vi = i; }
        }
    }
    startVoice(V[vi], note, vel, false);
  }

  void noteOff(int note) {
    if (nv == 1) {
      Voice &v = V[0];
      stackRemove(note);
      if (v.note != note || !v.gate) return;
      if (nStack > 0) {  // まだ押している鍵に戻る
        v.note = stack[nStack - 1];
        v.target = (float)v.note;
      } else if (sustain) v.held = true;
      else release(v);
      return;
    }
    for (int i = 0; i < nv; i++)
      if (V[i].active && V[i].gate && !V[i].held && V[i].note == note) {
        if (sustain) V[i].held = true;
        else release(V[i]);
      }
  }

  void handle(uint8_t type, uint8_t a, uint8_t b) {
    switch (type) {
      case EV_ON:
        if (b) noteOn(a, (float)b * (1.0f / 127.0f));
        else noteOff(a);
        break;
      case EV_OFF: noteOff(a); break;
      case EV_BEND: bend = ((float)(((int)b << 7) | a) - 8192.0f) * (1.0f / 8192.0f); break;
      case EV_MODW: modw = (float)a * (1.0f / 127.0f); break;
      case EV_SUS:
        sustain = a >= 64;
        if (!sustain)
          for (int i = 0; i < nv; i++)
            if (V[i].held) release(V[i]);
        break;
      case EV_ALLOFF:
        nStack = 0;
        sustain = false;
        for (int i = 0; i < nv; i++) release(V[i]);
        break;
    }
  }

  void switchModel(int m) {
    if (m < 0 || m >= N_MODELS) m = 0;
    cur = m;
    nv = MODELS[m].voices;
    for (int i = 0; i < MAXV; i++) V[i] = Voice();
    for (int i = 0; i < MAXV; i++) {  // VCO は位相をばらしておく
      V[i].ph1 = 0.5f + 0.5f * rnd();
      V[i].ph2 = 0.5f + 0.5f * rnd();
      V[i].ph3 = 0.5f + 0.5f * rnd();
    }
    nStack = 0;
    sustain = false;
    everPlayed = false;
    bend = 0.0f;
    memset(cbuf, 0, sizeof(cbuf));
    delayOn = false;
    dlp = 0.0f;
    lastGen = 0;  // derive() をやり直す
    cutS = P[P_CUT];
  }

  SYNTH_INL float tap(float delay) const {  // コーラスの読み出し（delay はサンプル数）
    float rp = (float)cw - delay + (float)CN;
    int i0 = (int)rp;
    float fr = rp - (float)i0;
    float a = cbuf[i0 & (CN - 1)], b = cbuf[(i0 + 1) & (CN - 1)];
    return a + (b - a) * fr;
  }

  static constexpr int QN = 64, CN = 1024, DN = 32768;
  volatile uint32_t q[QN];
  volatile uint32_t qHead = 0, qTail = 0;

  Voice V[MAXV];
  int cur = -1, nv = 1;
  uint32_t lastGen = 0, clock = 0;
  uint8_t stack[16];
  int nStack = 0;
  bool sustain = false, everPlayed = false, delayOn = false;
  float bend = 0, bendS = 0, modw = 0, pitchMod = 0;
  float cutS = 0.5f, volS = 0;
  float lfoPh = 0, lfoVal = 0, lfoSH = 0;
  float choPh = 0, choPh2 = 0;
  float cbuf[CN];
  int cw = 0;
  int16_t dbuf[DN];
  int dw = 0;
  float dlenS = 12000, dlp = 0;
};
