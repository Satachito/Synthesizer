// パラメータと機種（モデル）の定義。エンジン・Web 画面・MIDI が共通で使う
// 値はすべて float。つまみ類は 0..1、切り替え類は 0..(steps-1) の整数値
#pragma once
#include <stdint.h>

// X(名前, Web/保存用のキー, steps(0 = つまみ), MIDI CC(0 = なし), 既定値)
#define SYNTH_PARAMS(X) \
  X(O1WAVE, "o1wave", 4, 0, 0)      /* 0 saw 1 pulse 2 tri 3 sine */ \
  X(O1OCT, "o1oct", 5, 0, 2)        /* 0..4 = -2..+2 オクターブ */ \
  X(O1LVL, "o1lvl", 0, 0, 0.8f) \
  X(O2WAVE, "o2wave", 4, 0, 0) \
  X(O2OCT, "o2oct", 5, 0, 2) \
  X(O2SEMI, "o2semi", 25, 0, 12)    /* 0..24 = -12..+12 半音 */ \
  X(O2DET, "o2det", 0, 0, 0.5f)     /* ±50 セント */ \
  X(O2LVL, "o2lvl", 0, 0, 0) \
  X(O3WAVE, "o3wave", 4, 0, 1) \
  X(O3OCT, "o3oct", 5, 0, 1) \
  X(O3DET, "o3det", 0, 0, 0.5f) \
  X(O3LVL, "o3lvl", 0, 0, 0) \
  X(PW, "pw", 0, 0, 0)              /* 0 = 50% .. 1 = 95% */ \
  X(PWM, "pwm", 0, 0, 0)            /* LFO -> パルス幅 */ \
  X(NOISE, "noise", 0, 0, 0) \
  X(RING, "ring", 0, 0, 0)          /* OSC1 x OSC2 */ \
  X(PMENV, "pmenv", 0, 0, 0)        /* フィルター EG -> OSC1 の音程 */ \
  X(PMOSC, "pmosc", 0, 0, 0)        /* OSC2 -> OSC1 の FM */ \
  X(CUT, "cut", 0, 74, 0.6f) \
  X(RES, "res", 0, 71, 0.2f) \
  X(FENV, "fenv", 0, 0, 0.4f) \
  X(FKBD, "fkbd", 0, 0, 0.5f) \
  X(FLFO, "flfo", 0, 0, 0) \
  X(FMODE, "fmode", 0, 0, 0)        /* SEM: 0 LP .. 0.5 notch .. 1 HP */ \
  X(DRIVE, "drive", 0, 0, 0.3f) \
  X(HPF, "hpf", 0, 0, 0) \
  X(HPRES, "hpres", 0, 0, 0)        /* MS だけ */ \
  X(FA, "fa", 0, 0, 0) \
  X(FD, "fd", 0, 0, 0.5f) \
  X(FS, "fs", 0, 0, 0.4f) \
  X(FR, "fr", 0, 0, 0.4f) \
  X(AA, "aa", 0, 73, 0) \
  X(AD, "ad", 0, 75, 0.5f) \
  X(AS, "as", 0, 79, 0.8f) \
  X(AR, "ar", 0, 72, 0.3f) \
  X(VELS, "vel", 0, 0, 0.4f)        /* ベロシティの効き */ \
  X(ACC, "acc", 0, 0, 0.5f)         /* ACID: アクセントの深さ */ \
  X(LRATE, "lrate", 0, 76, 0.5f) \
  X(LWAVE, "lwave", 4, 0, 0)        /* 0 tri 1 saw 2 square 3 S&H */ \
  X(LPITCH, "lpitch", 0, 77, 0) \
  X(GLIDE, "glide", 0, 5, 0) \
  X(LEGATO, "legato", 2, 0, 0)      /* モノ機種: 1 = 重ねて弾いたときだけグライド、EG は掛け直さない */ \
  X(CHO, "cho", 4, 93, 0)           /* 0 off 1 I 2 II 3 ensemble */ \
  X(DTIME, "dtime", 0, 0, 0.4f) \
  X(DFB, "dfb", 0, 0, 0.3f) \
  X(DMIX, "dmix", 0, 91, 0) \
  X(VOL, "vol", 0, 7, 0.7f)

enum ParamId : uint8_t {
#define X(id, key, steps, cc, def) P_##id,
  SYNTH_PARAMS(X)
#undef X
  NP
};

struct ParamInfo {
  const char *key;
  uint8_t steps;
  uint8_t cc;
  float def;
};

static const ParamInfo PARAM_INFO[NP] = {
#define X(id, key, steps, cc, def) { key, steps, cc, def },
  SYNTH_PARAMS(X)
#undef X
};

// ---- 機種 ----
enum FilterType : uint8_t {
  FILT_LADDER,  // 24dB/oct トランジスタラダー
  FILT_SVF,     // 12dB/oct ステートバリアブル（LP〜HP を連続で切り替え）
  FILT_MS,      // 12dB HPF + 12dB LPF、どちらも発振する
};
enum ModelFlag : uint8_t {
  MF_DCO = 1,   // 鍵盤を押すたびに位相をそろえる（DCO）
  MF_ACID = 2,  // ベロシティ 100 以上でアクセント
};

struct PatchItem {
  uint8_t id;
  float v;
};
#define PATCH_END { 255, 0 }

struct ModelDef {
  const char *name;
  uint8_t voices;  // 1 = モノ
  uint8_t filt;
  uint8_t flags;
  float resMax;    // RES = 1 のときのフィードバック量（ラダーは 4 で発振）
  float comp;      // レゾナンスを上げたときの音量の落ち込みをどれだけ補うか
  const PatchItem *patch;
};

static const PatchItem PATCH_MINI[] = {
  { P_O1WAVE, 0 }, { P_O1OCT, 1 }, { P_O1LVL, 0.8f },
  { P_O2WAVE, 0 }, { P_O2OCT, 1 }, { P_O2DET, 0.57f }, { P_O2LVL, 0.8f },
  { P_O3WAVE, 0 }, { P_O3OCT, 0 }, { P_O3DET, 0.44f }, { P_O3LVL, 0.6f },
  { P_CUT, 0.42f }, { P_RES, 0.25f }, { P_FENV, 0.45f }, { P_FKBD, 0.4f }, { P_DRIVE, 0.55f },
  { P_FA, 0 }, { P_FD, 0.5f }, { P_FS, 0.3f }, { P_FR, 0.35f },
  { P_AA, 0 }, { P_AD, 0.5f }, { P_AS, 0.85f }, { P_AR, 0.3f },
  { P_GLIDE, 0.2f }, { P_LEGATO, 1 }, PATCH_END
};
static const PatchItem PATCH_ACID[] = {
  { P_O1WAVE, 0 }, { P_O1OCT, 1 }, { P_O1LVL, 0.9f },
  { P_CUT, 0.3f }, { P_RES, 0.8f }, { P_FENV, 0.5f }, { P_FKBD, 0.2f }, { P_DRIVE, 0.6f },
  { P_FA, 0 }, { P_FD, 0.42f }, { P_FS, 0 }, { P_FR, 0.42f },
  { P_AA, 0 }, { P_AD, 0.6f }, { P_AS, 0.7f }, { P_AR, 0.15f },
  { P_ACC, 0.7f }, { P_VELS, 0 }, { P_GLIDE, 0.3f }, { P_LEGATO, 1 },
  { P_DTIME, 0.45f }, { P_DFB, 0.35f }, { P_DMIX, 0.15f }, PATCH_END
};
static const PatchItem PATCH_JUNO[] = {
  { P_O1WAVE, 0 }, { P_O1OCT, 2 }, { P_O1LVL, 0.7f },
  { P_O2WAVE, 1 }, { P_O2OCT, 2 }, { P_O2LVL, 0.5f },
  { P_O3WAVE, 1 }, { P_O3OCT, 1 }, { P_O3LVL, 0.4f },
  { P_PWM, 0.5f }, { P_LRATE, 0.4f }, { P_HPF, 0.1f },
  { P_CUT, 0.55f }, { P_RES, 0.15f }, { P_FENV, 0.3f }, { P_FKBD, 0.6f }, { P_DRIVE, 0.2f },
  { P_FA, 0.05f }, { P_FD, 0.55f }, { P_FS, 0.6f }, { P_FR, 0.5f },
  { P_AA, 0.05f }, { P_AD, 0.55f }, { P_AS, 0.8f }, { P_AR, 0.5f },
  { P_CHO, 1 }, PATCH_END
};
static const PatchItem PATCH_PRO5[] = {
  { P_O1WAVE, 0 }, { P_O1OCT, 2 }, { P_O1LVL, 0.75f },
  { P_O2WAVE, 0 }, { P_O2OCT, 2 }, { P_O2DET, 0.56f }, { P_O2LVL, 0.75f },
  { P_CUT, 0.45f }, { P_RES, 0.25f }, { P_FENV, 0.45f }, { P_FKBD, 0.5f }, { P_DRIVE, 0.3f },
  { P_FA, 0.2f }, { P_FD, 0.55f }, { P_FS, 0.55f }, { P_FR, 0.45f },
  { P_AA, 0.1f }, { P_AD, 0.55f }, { P_AS, 0.8f }, { P_AR, 0.45f }, PATCH_END
};
static const PatchItem PATCH_MS[] = {
  { P_O1WAVE, 0 }, { P_O1OCT, 1 }, { P_O1LVL, 0.8f },
  { P_O2WAVE, 1 }, { P_O2OCT, 2 }, { P_O2DET, 0.53f }, { P_O2LVL, 0.6f },
  { P_HPF, 0.15f }, { P_HPRES, 0.4f },
  { P_CUT, 0.45f }, { P_RES, 0.65f }, { P_FENV, 0.5f }, { P_FKBD, 0.4f }, { P_DRIVE, 0.6f },
  { P_FA, 0 }, { P_FD, 0.5f }, { P_FS, 0.2f }, { P_FR, 0.4f },
  { P_AA, 0 }, { P_AD, 0.5f }, { P_AS, 0.8f }, { P_AR, 0.3f },
  { P_GLIDE, 0.15f }, PATCH_END
};
static const PatchItem PATCH_SEM[] = {
  { P_O1WAVE, 0 }, { P_O1OCT, 2 }, { P_O1LVL, 0.7f },
  { P_O2WAVE, 1 }, { P_O2OCT, 2 }, { P_O2DET, 0.55f }, { P_O2LVL, 0.6f }, { P_PW, 0.3f },
  { P_CUT, 0.5f }, { P_RES, 0.35f }, { P_FENV, 0.35f }, { P_FKBD, 0.5f }, { P_DRIVE, 0.3f },
  { P_FA, 0 }, { P_FD, 0.55f }, { P_FS, 0.4f }, { P_FR, 0.45f },
  { P_AA, 0.02f }, { P_AD, 0.55f }, { P_AS, 0.8f }, { P_AR, 0.45f }, PATCH_END
};
static const PatchItem PATCH_STR[] = {
  { P_O1WAVE, 0 }, { P_O1OCT, 2 }, { P_O1LVL, 0.6f },
  { P_O2WAVE, 0 }, { P_O2OCT, 3 }, { P_O2DET, 0.52f }, { P_O2LVL, 0.45f },
  { P_CUT, 0.72f }, { P_RES, 0 }, { P_FENV, 0 }, { P_FKBD, 0.3f }, { P_DRIVE, 0.2f }, { P_HPF, 0.25f },
  { P_AA, 0.48f }, { P_AD, 0.5f }, { P_AS, 1 }, { P_AR, 0.55f },
  { P_VELS, 0 }, { P_CHO, 3 }, PATCH_END
};

static const ModelDef MODELS[] = {
  //  name       voices filter       flags    resMax comp
  { "MINI", 1, FILT_LADDER, 0, 4.3f, 0.0f, PATCH_MINI },      // Minimoog 風: 3 VCO + ラダー、モノ
  { "ACID", 1, FILT_LADDER, MF_ACID, 3.9f, 0.3f, PATCH_ACID }, // TB-303 風: アクセントとスライド
  { "JUNO", 6, FILT_LADDER, MF_DCO, 4.3f, 0.5f, PATCH_JUNO },  // Juno 風: DCO + サブ + コーラス、6 音
  { "PRO-5", 5, FILT_LADDER, 0, 4.3f, 0.4f, PATCH_PRO5 },      // Prophet-5 風: 2 VCO + ポリモジュレーション、5 音
  { "MS", 1, FILT_MS, 0, 1.0f, 0.0f, PATCH_MS },               // MS-20 風: HPF + LPF、リングモジュレーター
  { "SEM", 4, FILT_SVF, 0, 1.0f, 0.0f, PATCH_SEM },            // Oberheim SEM 風: 12dB マルチモード、4 音
  { "STRINGS", 8, FILT_SVF, 0, 1.0f, 0.0f, PATCH_STR },        // ストリングマシン風: アンサンブル、8 音
};
const int N_MODELS = sizeof(MODELS) / sizeof(MODELS[0]);

// その機種の工場出荷時の音色を dst[NP] に入れる
static inline void modelDefaults(int m, float *dst) {
  for (int i = 0; i < NP; i++) dst[i] = PARAM_INFO[i].def;
  for (const PatchItem *it = MODELS[m].patch; it->id != 255; it++) dst[it->id] = it->v;
}

// ---- プリセット（フラッシュに残す形）----
// .ino の中で定義すると Arduino が自動で作る関数宣言より後ろになってしまうので、ここに置く
const int N_PRESETS = 32;
struct Preset {
  uint8_t used;      // 1 = 入っている
  uint8_t model;
  char name[24];     // UTF-8、終端つき
  uint16_t v[NP];    // つまみは x10000、切り替えはそのままの整数
};
