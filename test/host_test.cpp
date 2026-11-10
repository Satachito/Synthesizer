// エンジンを PC で鳴らして WAV に書き出す。機種ごとに 1 ファイル
//   c++ -std=c++17 -O2 -I.. host_test.cpp -o host_test && ./host_test out/
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <string>
#include <vector>
#include "synth_engine.h"

static Synth synth;

static void writeWav(const std::string &path, const std::vector<int16_t> &s) {
  FILE *f = fopen(path.c_str(), "wb");
  if (!f) { perror(path.c_str()); exit(1); }
  uint32_t n = s.size() * 2, sr = 48000, br = sr * 4, riff = 36 + n, fmt = 16;
  uint16_t pcm = 1, ch = 2, ba = 4, bits = 16;
  fwrite("RIFF", 1, 4, f); fwrite(&riff, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
  fwrite(&fmt, 4, 1, f); fwrite(&pcm, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&sr, 4, 1, f);
  fwrite(&br, 4, 1, f); fwrite(&ba, 2, 1, f); fwrite(&bits, 2, 1, f);
  fwrite("data", 1, 4, f); fwrite(&n, 4, 1, f); fwrite(s.data(), 2, s.size(), f);
  fclose(f);
}

struct Stat { double sum2 = 0; int peak = 0; long n = 0, clip = 0; double sec = 0; };

static void run(std::vector<int16_t> &out, float seconds, Stat &st) {
  int blocks = (int)(seconds * Synth::SR / Synth::BLOCK);
  int16_t buf[Synth::BLOCK * 2];
  auto t0 = std::chrono::steady_clock::now();
  for (int b = 0; b < blocks; b++) {
    synth.render(buf);
    for (int i = 0; i < Synth::BLOCK * 2; i++) {
      int v = buf[i];
      st.sum2 += (double)v * v;
      if (abs(v) > st.peak) st.peak = abs(v);
      if (abs(v) >= 31990) st.clip++;
      st.n++;
      out.push_back(buf[i]);
    }
  }
  st.sec += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

int main(int argc, char **argv) {
  std::string dir = argc > 1 ? argv[1] : ".";
  synth.begin();
  int fail = 0;
  {  // 音程: サイン波だけにして A4 = 440Hz になるか（ゼロクロスを数える）
    modelDefaults(5, synth.P);
    synth.P[P_O1WAVE] = 3; synth.P[P_O2LVL] = 0; synth.P[P_CUT] = 1; synth.P[P_FENV] = 0; synth.P[P_AS] = 1;
    synth.model = 5;
    synth.gen = synth.gen + 1;
    std::vector<int16_t> out;
    Stat st;
    synth.post(Synth::EV_ON, 69, 100);
    run(out, 2.0f, st);
    synth.post(Synth::EV_ALLOFF);
    int zc = 0;
    for (size_t i = 48000 * 2 + 2; i < out.size(); i += 2) zc += out[i - 2] < 0 && out[i] >= 0;
    printf("tuning: A4 -> %d Hz %s\n", zc, zc >= 439 && zc <= 441 ? "ok" : "<-- CHECK");
    if (zc < 439 || zc > 441) fail = 1;
    run(out, 1.0f, st);
  }
  for (int m = 0; m < N_MODELS; m++) {
    modelDefaults(m, synth.P);
    synth.model = m;
    synth.gen = synth.gen + 1;
    std::vector<int16_t> out;
    Stat st;
    bool mono = MODELS[m].voices == 1;
    if (mono) {  // フレーズ（重ねて弾くところ、アクセント付き）
      const int seq[] = { 36, 36, 48, 39, 36, 43, 46, 48 };
      for (int r = 0; r < 2; r++)
        for (int i = 0; i < 8; i++) {
          synth.post(Synth::EV_ON, seq[i], i % 3 == 0 ? 120 : 80);
          run(out, 0.14f, st);
          if (i % 4 == 1) {  // 次の音と重ねる
            synth.post(Synth::EV_ON, seq[(i + 1) % 8], 80);
            run(out, 0.14f, st);
            synth.post(Synth::EV_OFF, seq[(i + 1) % 8]);
          }
          synth.post(Synth::EV_OFF, seq[i]);
          run(out, 0.06f, st);
        }
    } else {  // 和音。最後は発音数を超えるまで重ねる
      const int ch1[] = { 48, 55, 60, 64, 67 }, ch2[] = { 45, 52, 57, 60, 64, 69, 72, 76, 81, 84 };
      for (int n : ch1) synth.post(Synth::EV_ON, n, 90);
      run(out, 1.5f, st);
      for (int n : ch1) synth.post(Synth::EV_OFF, n);
      run(out, 0.5f, st);
      for (int n : ch2) { synth.post(Synth::EV_ON, n, 110); run(out, 0.1f, st); }
      run(out, 1.0f, st);
      synth.post(Synth::EV_ALLOFF);
    }
    run(out, 1.5f, st);
    // つまみを端まで回しても壊れないか: レゾナンス最大でカットオフを上下させる
    Stat st2;
    synth.P[P_RES] = 1.0f; synth.P[P_HPRES] = 1.0f; synth.P[P_DRIVE] = 1.0f; synth.P[P_NOISE] = 0.5f;
    synth.P[P_PMOSC] = 1.0f; synth.P[P_RING] = 1.0f; synth.P[P_DMIX] = 1.0f; synth.P[P_DFB] = 1.0f; synth.P[P_CHO] = 3;
    for (int n = 40; n < 40 + 8; n++) synth.post(Synth::EV_ON, n, 127);
    for (int i = 0; i < 60; i++) {
      synth.P[P_CUT] = (i % 20) / 19.0f;
      synth.P[P_HPF] = 1.0f - (i % 20) / 19.0f;
      synth.gen = synth.gen + 1;
      run(out, 0.05f, st2);
    }
    synth.post(Synth::EV_ALLOFF);
    run(out, 0.3f, st2);
    writeWav(dir + "/" + MODELS[m].name + ".wav", out);
    double rms = sqrt(st.sum2 / st.n), total = st.sec + st2.sec, audio = (st.n + st2.n) / 2 / 48000.0;
    bool bad = st.peak < 2000 || st.clip > st.n / 50;
    printf("%-8s peak %5d  rms %5.0f  clipped %5.2f%%  | extreme: peak %5d | host %.1fx realtime %s\n", MODELS[m].name, st.peak, rms,
           100.0 * st.clip / st.n, st2.peak, audio / total, bad ? "  <-- CHECK" : "");
    if (bad) fail = 1;
  }
  return fail;
}
