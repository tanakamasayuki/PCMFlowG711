# PCMFlowG711

> English: [README.md](README.md)

[PCMFlow](https://github.com/tanakamasayuki/PCMFlow) 用のオプション **G.711**(μ-law / A-law)コーデックアドオン。**リアルタイム双方向音声**(VoIP / ESP-NOW トランシーバ / WebSocket / UDP 音声リンク)向け。

ITU-T G.711 の**クリーンルーム MIT 実装**。第三者ソースの vendor は一切なく、ライブラリ全体が単独著者の MIT。組み込み時にも標準 MIT 表記以外の attribution は不要。エンコーダ + デコーダ + 両 variant をすべて含めて Flash 4 KB 以下。

詳細は [SPEC.ja.md](SPEC.ja.md) を参照。

---

## 構成

| クラス | 方向 | 担い手 | インタフェース |
|--------|------|--------|----------------|
| `G711Encoder` | PCM → G.711 バイト | 生バイト(RTP / ESP-NOW / UDP / WebSocket) | `PCMSink` |
| `G711Decoder` | G.711 バイト → PCM | 生バイト | `PCMSource` |

PCM 1 サンプル ↔ G.711 1 バイト。コーデック内部にフレーミングはないので、チャンクサイズは運搬媒体側が決める(典型は RTP 標準の 160 バイト = 20 ms @ 8 kHz)。**μ-law(PCMU、RTP payload type 0)**と **A-law(PCMA、RTP payload type 8)**の両方をサポート、`begin()` で選択。

WAV コンテナ(`WAVE_FORMAT_MULAW` / `WAVE_FORMAT_ALAW`)は**初版では対象外** — [SPEC §「将来対応」](SPEC.ja.md#将来対応) 参照。

---

## PCMFlow コーデックファミリー

PCMFlowG711 は PCMFlow のオプションコーデックアドオン群の 1 つ。帯域 / フットプリント / 音質のバランスで使い分ける:

| | **PCMFlowG711**(本ライブラリ) | [PCMFlowG722](https://github.com/tanakamasayuki/PCMFlowG722) | [PCMFlowOpus](https://github.com/tanakamasayuki/PCMFlowOpus) |
|---|---|---|---|
| 帯域 | 狭帯域(8 kHz / ≤ 3.4 kHz) | **広帯域(16 kHz / ≤ 7 kHz)** | 狭帯域/広帯域/全帯域(8〜48 kHz) |
| ビットレート(音声) | 64 kbps 固定 | 48 / 56 / 64 kbps | 16〜32 kbps 程度 |
| 生 16-bit PCM 比 | 2× | 4× | 10〜15× |
| コーデック Flash | **< 4 KB** | ~10 KB(見込み) | ~150〜180 KB |
| コーデック CPU | ほぼゼロ | 低 | M0/M3 クラスでは無視できない |
| 特許 / ライセンス | なし(1972 規格、満了済) | なし(1988 規格、満了済) | royalty-free 特許許諾、BSD-3-Clause ソース |
| 音質 | 電話品質(toll-grade) | HD voice(広帯域電話品質) | 広帯域 / 全帯域、圧倒的に良い |

G.711 を選ぶケース:
- 電波 / ネットワーク帯域に 64 kbps の余裕がある
- 大きなアプリと共存する / RAM 制約の厳しいボードで、コーデックフットプリント最小が最優先
- ライセンスを MIT 単一・第三者 attribution なしで保ちたい

同じ 64 kbps で **HD voice 広帯域音声**が欲しく、フットプリントの中程度の増加を許容できるなら [G.722(PCMFlowG722)](https://github.com/tanakamasayuki/PCMFlowG722) を選ぶ。G.722 はファミリー内で G.711 と Opus の中間に位置する。

帯域節約 / 全帯域音質が必要なら [Opus(PCMFlowOpus)](https://github.com/tanakamasayuki/PCMFlowOpus) を選ぶ。

---

## 主目的ユースケース — ESP-NOW トランシーバ

ESP-NOW は 1 パケット最大 250 byte ペイロード。20 ms G.711(8 kHz × 8 bit)= ちょうど 160 byte、ヘッダ含めて余裕で収まる。同じ 20 ms の生 8 kHz mono 16-bit PCM は 320 byte なので G.711 で **2×** 圧縮。

```cpp
#include <PCMFlow.h>
#include <PCMFlowG711.h>
#include <esp_now.h>

G711Encoder enc;
G711Decoder dec;
PCMFlow audio;

void setup() {
    audio.setOutputFormat({8000, 1, 16});
    audio.setInputSource(dec);

    dec.begin({8000, 1, 16}, G711Variant::MuLaw);
    enc.begin({8000, 1, 16}, G711Variant::MuLaw);

    esp_now_init();
    esp_now_register_recv_cb(onEspNowRecv);
}

// ESP-NOW 受信コールバック: バイト列をデコーダへ
void onEspNowRecv(const uint8_t *mac, const uint8_t *data, int len) {
    dec.decode(data, len, /*pcm_out=*/nullptr, /*max_frames=*/0);
    // PCMFlow の pump() が PCM を取り出して I2S/DAC へ流す
}
```

エンドツーエンドのトランシーバスケッチ(マイク↔ESP-NOW↔DAC)は [examples/EspNowTransceiver/](examples/EspNowTransceiver/) に同梱。1 台で完結する動作確認用に [examples/MicLoopback/](examples/MicLoopback/) もある。

---

## 依存

- **[PCMFlow](https://github.com/tanakamasayuki/PCMFlow)** — `PCMSource` / `PCMSink` / `ByteStream` / `ByteSink` / 再生パイプラインを提供。`library.properties` の `depends=PCMFlow` で宣言。

これだけ。Vendor したコーデックも外部ライブラリもなし。

---

## 対応プラットフォーム

親 PCMFlow と同じ:`library.properties` は `architectures=*`、ただし実用ターゲットは 32-bit MCU。

実用例: ESP32 / ESP32-S3 / ESP32-C3 / ESP32-C6 / ESP32-P4、RP2040 / RP2350、Teensy 4.x、STM32 F4 以上、nRF52。G.711 は十分軽いので、PCMFlow 側に RAM の余裕があれば AVR(Uno / Mega / Nano)や SAMD21 級でも実用可能。

暫定フットプリント(エンコーダ + デコーダ、両 variant): **Flash < 4 KB / RAM ≤ 32 B per instance**。デコーダのみのビルドではエンコーダ側がリンク時に落ちて flash がさらに減る。詳細目標は [SPEC.ja.md §6](SPEC.ja.md)。

---

## ライセンス

PCMFlowG711: **MIT** ([LICENSE](LICENSE))。

このライブラリは ITU-T G.711 アルゴリズム(1972 年の公開規格、数学的写像のみが規範で著作権対象外)の**クリーンルーム実装**。**第三者ソースの vendor は一切なし**なので、`src/external/` も追加ライセンスファイルも存在せず、標準 MIT 表記以外の attribution は不要。

---

## テスト

```sh
cd tests
uv run --env-file .env pytest                  # host (デフォルト)
uv run --env-file .env pytest --profile=esp32  # 実機 ESP32
```

詳細は [tests/README.ja.md](tests/README.ja.md)。親 PCMFlow と同じ規約(pytest-embedded + Arduino CLI、`lang-ship:host` + `esp32:esp32:esp32` プロファイル)。
