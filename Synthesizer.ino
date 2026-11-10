/*
  Synthesizer — Raspberry Pi Pico 2 W のオールドシンセ風シンセサイザー
  機種を切り替えて使う（MINI / ACID / JUNO / PRO-5 / MS / SEM / STRINGS）。鍵盤は MIDI、つまみは Wi-Fi の画面

  ビルド:
    arduino-cli compile --fqbn rp2040:rp2040:rpipico2w:opt=Optimize2 .
  書き込み（BOOTSEL を押しながら USB をつなぐ）:
    arduino-cli upload --fqbn rp2040:rp2040:rpipico2w:opt=Optimize2 -p /Volumes/RP2350 .

  配線（Digimin の DAC 版と同じ）
    PCM5102A モジュール  LCK -> GP0（物理 1）,  BCK -> GP1（物理 2）,  DIN -> GP2（物理 4）
                         SCK -> GND,  VIN -> VSYS（物理 39）,  GND -> GND
                         裏のジャンパー: H1 / H2 / H4 = L、H3 = H
    MIDI IN（DIN）       フォトカプラ H11L1M の出力（4 番）-> GP5（物理 7）。UART1 の RX、31250bps
                         電源とプルアップは 3V3(OUT)（物理 36）から。基板の配置は README.md
    USB                  PC や iPad につなぐと USB-MIDI 機器として見える（Mac での名前は "Pico 2W virtual-cable"）

  Wi-Fi: アクセスポイント "Synth-XXXX" につないで http://192.168.42.1/ を開く（何を開いてもここに来る）

  コアの分担
    Core0: Wi-Fi、Web 画面、MIDI の受信、音色の保存
    Core1: 音の計算（synth_engine.h）と I2S 出力だけ
*/
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <I2S.h>
#include <MIDIUSB.h>
#include "pico/unique_id.h"
#include "synth_engine.h"
#include "web_ui.h"

const uint8_t PIN_I2S_LCK = 0;  // LCK, BCK の順に隣り合わせ
const uint8_t PIN_I2S_DIN = 2;
const uint8_t PIN_MIDI_TX = 4;  // 使わないが UART1 の TX として確保される
const uint8_t PIN_MIDI_RX = 5;
const uint8_t PIN_SMPS_MODE = 65;  // Wi-Fi チップの GPIO1 = 電源回路のモード切り替え（HIGH で PWM モード = ノイズが減る）

const char *WIFI_SSID_BASE = "Synth";
const char *WIFI_PASS = "";  // 8 文字以上を入れるとパスワード付きになる。空ならパスワードなし

Synth synth;
I2S i2s(OUTPUT);
WebServer server(80);
DNSServer dns;
String wifiSsid;

float patches[N_MODELS][NP];  // 機種ごとの編集中の音色。今の機種のぶんは synth.P が最新
int curModel = 0;
volatile bool engineReady = false;
volatile uint32_t loadPct = 0, loadPeakPct = 0;  // Core1 が書く
uint32_t midiCount = 0;

// ---------- 音色 ----------
void setParam(int id, float v) {
  if (id < 0 || id >= NP) return;
  int steps = PARAM_INFO[id].steps;
  if (steps) v = constrain((int)(v + 0.5f), 0, steps - 1);
  else v = constrain(v, 0.0f, 1.0f);
  synth.P[id] = v;
  synth.gen = synth.gen + 1;
}

void setModel(int m) {
  if (m < 0 || m >= N_MODELS || m == curModel) return;
  memcpy(patches[curModel], synth.P, sizeof(synth.P));
  curModel = m;
  memcpy(synth.P, patches[m], sizeof(synth.P));
  synth.model = m;
  synth.gen = synth.gen + 1;
}

// 保存（フラッシュ）。先頭に印、次に機種番号、そのあと 機種 x パラメータ を 16bit で並べる
const uint32_t STORE_MAGIC = 0x53594E00u + NP * 16 + N_MODELS;  // 並びが変わったら読まない
const int STORE_SIZE = 8 + N_MODELS * NP * 2;

void storeLoad() {
  for (int m = 0; m < N_MODELS; m++) modelDefaults(m, patches[m]);
  EEPROM.begin(STORE_SIZE);
  uint32_t magic;
  EEPROM.get(0, magic);
  if (magic == STORE_MAGIC) {
    int m0 = EEPROM.read(4);
    if (m0 < N_MODELS) curModel = m0;
    for (int m = 0; m < N_MODELS; m++)
      for (int i = 0; i < NP; i++) {
        uint16_t w;
        EEPROM.get(8 + (m * NP + i) * 2, w);
        float v = PARAM_INFO[i].steps ? (float)w : w / 10000.0f;
        float hi = PARAM_INFO[i].steps ? PARAM_INFO[i].steps - 1 : 1.0f;
        if (v <= hi) patches[m][i] = v;
      }
  }
  memcpy(synth.P, patches[curModel], sizeof(synth.P));
  synth.model = curModel;
  synth.gen = synth.gen + 1;
}

// 全機種の音色と、今どの機種かを保存する。書き込み中（数十 ms）は音が止まる
void storeSave() {
  memcpy(patches[curModel], synth.P, sizeof(synth.P));
  EEPROM.put(0, STORE_MAGIC);
  EEPROM.write(4, curModel);
  for (int m = 0; m < N_MODELS; m++)
    for (int i = 0; i < NP; i++) {
      float v = patches[m][i];
      uint16_t w = PARAM_INFO[i].steps ? (uint16_t)v : (uint16_t)(v * 10000.0f + 0.5f);
      EEPROM.put(8 + (m * NP + i) * 2, w);
    }
  EEPROM.commit();
}

// ---------- MIDI ----------
// チャンネルは見ない（オムニ）
void midiMessage(uint8_t status, uint8_t d1, uint8_t d2) {
  midiCount++;
  switch (status & 0xf0) {
    case 0x90: synth.post(Synth::EV_ON, d1, d2); break;
    case 0x80: synth.post(Synth::EV_OFF, d1); break;
    case 0xe0: synth.post(Synth::EV_BEND, d1, d2); break;
    case 0xc0: setModel(d1 % N_MODELS); break;  // プログラムチェンジで機種を切り替える
    case 0xb0:
      if (d1 == 1) synth.post(Synth::EV_MODW, d2);
      else if (d1 == 64) synth.post(Synth::EV_SUS, d2);
      else if (d1 == 120 || d1 == 123) synth.post(Synth::EV_ALLOFF);
      else
        for (int i = 0; i < NP; i++)
          if (PARAM_INFO[i].cc == d1) {
            int steps = PARAM_INFO[i].steps;
            setParam(i, steps ? (float)(d2 * steps / 128) : d2 / 127.0f);
            break;
          }
      break;
  }
}

// DIN の MIDI（バイト列）。ランニングステータスに対応
void midiByte(uint8_t b) {
  static uint8_t status = 0, d1 = 0;
  static bool have1 = false;
  if (b >= 0xf8) return;  // クロックなどのリアルタイムは無視
  if (b & 0x80) {
    status = b >= 0xf0 ? 0 : b;  // システムメッセージは読み飛ばす
    have1 = false;
    return;
  }
  if (!status) return;
  uint8_t hi = status & 0xf0;
  if (hi == 0xc0 || hi == 0xd0) {  // データ 1 バイト
    midiMessage(status, b, 0);
  } else if (!have1) {
    d1 = b;
    have1 = true;
  } else {
    have1 = false;
    midiMessage(status, d1, b);
  }
}

void midiPoll() {
  while (Serial2.available()) midiByte(Serial2.read());
  uint8_t pkt[4];
  for (int n = 0; n < 32 && MidiUSB.readPacket(pkt); n++) {
    uint8_t cin = pkt[0] & 0x0f;
    if (cin >= 0x8 && cin <= 0xe) midiMessage(pkt[1], pkt[2], pkt[3]);
  }
}

// ---------- Web ----------
String stateJson() {
  String s;
  s.reserve(600);
  s += "{\"m\":";
  s += curModel;
  s += ",\"p\":[";
  for (int i = 0; i < NP; i++) {
    if (i) s += ',';
    if (PARAM_INFO[i].steps) s += (int)synth.P[i];
    else s += String(synth.P[i], 3);
  }
  s += "],\"load\":";
  s += loadPct;
  s += ",\"peak\":";
  s += loadPeakPct;
  s += ",\"voices\":";
  s += synth.activeVoices;
  s += ",\"midi\":";
  s += midiCount;
  s += "}";
  return s;
}

String metaJson() {
  String s;
  s.reserve(900);
  s += "{\"keys\":[";
  for (int i = 0; i < NP; i++) {
    if (i) s += ',';
    s += '"';
    s += PARAM_INFO[i].key;
    s += '"';
  }
  s += "],\"steps\":[";
  for (int i = 0; i < NP; i++) {
    if (i) s += ',';
    s += PARAM_INFO[i].steps;
  }
  s += "],\"models\":[";
  for (int m = 0; m < N_MODELS; m++) {
    if (m) s += ',';
    s += "{\"name\":\"";
    s += MODELS[m].name;
    s += "\",\"voices\":";
    s += MODELS[m].voices;
    s += "}";
  }
  s += "],\"ssid\":\"";
  s += wifiSsid;
  s += "\"}";
  return s;
}

void sendState() {
  server.send(200, "application/json", stateJson());
}

void webInit() {
  server.on("/", []() {
    server.send(200, "text/html; charset=utf-8", INDEX_HTML);
  });
  server.on("/api/meta", []() {
    server.send(200, "application/json", metaJson());
  });
  server.on("/api/state", sendState);
  // /api/set?cut=0.53&res=0.2 のようにキーをそのまま並べる
  server.on("/api/set", []() {
    for (int a = 0; a < server.args(); a++)
      for (int i = 0; i < NP; i++)
        if (server.argName(a) == PARAM_INFO[i].key) {
          setParam(i, server.arg(a).toFloat());
          break;
        }
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/model", []() {
    setModel(server.arg("m").toInt());
    sendState();
  });
  // 画面の鍵盤。v = 0 で離す
  server.on("/api/note", []() {
    int n = server.arg("n").toInt(), v = server.arg("v").toInt();
    if (n >= 0 && n < 128) synth.post(v > 0 ? Synth::EV_ON : Synth::EV_OFF, n, constrain(v, 0, 127));
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/panic", []() {
    synth.post(Synth::EV_ALLOFF);
    server.send(200, "text/plain", "ok");
  });
  server.on("/api/save", []() {
    storeSave();
    sendState();
  });
  // 今の機種を工場出荷時の音色に戻す（保存はしない）
  server.on("/api/init", []() {
    modelDefaults(curModel, synth.P);
    synth.gen = synth.gen + 1;
    sendState();
  });
  server.onNotFound([]() {  // キャプティブポータル: 何を開いても操作画面へ
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
    server.send(302, "text/plain", "");
  });
}

void wifiStart() {
  pico_unique_board_id_t uid;
  pico_get_unique_board_id(&uid);
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%02X%02X", uid.id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES - 2], uid.id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES - 1]);
  wifiSsid = String(WIFI_SSID_BASE) + "-" + suffix;
  WiFi.mode(WIFI_AP);
  IPAddress apIp(192, 168, 42, 1);
  WiFi.softAPConfig(apIp, apIp, IPAddress(255, 255, 255, 0));
  WiFi.softAP(wifiSsid.c_str(), strlen(WIFI_PASS) >= 8 ? WIFI_PASS : nullptr);
  dns.start(53, "*", apIp);
  server.begin();
}

// ---------- Core0 ----------
void setup() {
  Serial.begin(115200);
  synth.begin();
  storeLoad();
  engineReady = true;

  MidiUSB.setName("Pico Synth");
  MidiUSB.begin();
  Serial2.setTX(PIN_MIDI_TX);
  Serial2.setRX(PIN_MIDI_RX);
  Serial2.begin(31250);
  gpio_pull_up(PIN_MIDI_RX);  // 何もつないでいないときにノイズを MIDI として読まないように

  webInit();
  wifiStart();
  pinMode(PIN_SMPS_MODE, OUTPUT);
  digitalWrite(PIN_SMPS_MODE, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  midiPoll();
  dns.processNextRequest();
  server.handleClient();
  midiPoll();

  // 診断: 1 秒ごとに負荷を USB シリアルに出す
  static uint32_t lastRep = 0;
  if (millis() - lastRep >= 1000) {
    lastRep = millis();
    Serial.printf("model %s voices %d load %lu%% peak %lu%%\n", MODELS[curModel].name, (int)synth.activeVoices, (unsigned long)loadPct, (unsigned long)loadPeakPct);
  }

  // LED: 音が出ている間は点灯。Wi-Fi チップ経由なので変わったときだけ書く
  static bool ledNow = false;
  bool w = synth.activeVoices > 0;
  if (w != ledNow) {
    ledNow = w;
    digitalWrite(LED_BUILTIN, w ? HIGH : LOW);
  }
}

// ---------- Core1: 音 ----------
void setup1() {
  while (!engineReady) delay(1);
  i2s.setBCLK(PIN_I2S_LCK);  // swapClocks() で 指定ピン = LCK、その次 = BCK になる
  i2s.swapClocks();
  i2s.setDATA(PIN_I2S_DIN);
  i2s.setBitsPerSample(16);
  i2s.setBuffers(8, 32);  // 32 フレーム x 8。遅れは 5ms ほど
  i2s.begin((long)Synth::SR);
}

void loop1() {
  static int16_t buf[Synth::BLOCK * 2];
  static uint32_t words[Synth::BLOCK];
  static uint32_t sumUs = 0, peakUs = 0, blocks = 0;
  const uint32_t blockUs = (uint32_t)(Synth::BLOCK * 1000000.0f / Synth::SR);

  uint32_t t0 = micros();
  synth.render(buf);
  uint32_t dt = micros() - t0;
  sumUs += dt;
  if (dt > peakUs) peakUs = dt;
  if (++blocks >= 1500) {  // 0.5 秒ごとに負荷を出す。平均が 100% に近づくと音が途切れる
    // 最大はつまみを動かした瞬間（derive() の再計算）に 100% を超えるが、送信バッファが 5ms あるので途切れない
    loadPct = sumUs * 100 / (blocks * blockUs);
    loadPeakPct = peakUs * 100 / blockUs;
    sumUs = peakUs = blocks = 0;
  }

  for (int i = 0; i < Synth::BLOCK; i++) words[i] = ((uint32_t)(uint16_t)buf[i * 2] << 16) | (uint16_t)buf[i * 2 + 1];
  const uint8_t *p = (const uint8_t *)words;
  size_t left = sizeof(words);
  while (left) {  // 送信バッファが空くまで待つ
    size_t n = i2s.write(p, left);
    p += n;
    left -= n;
  }
}
