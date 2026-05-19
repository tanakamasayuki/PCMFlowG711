# PCMFlowG711 仕様

> English: [SPEC.md](SPEC.md)

## 1. スコープ

**PCMFlowG711** は [PCMFlow](https://github.com/tanakamasayuki/PCMFlow) 向けのオプション G.711 コーデックアドオン。

> **同ファミリーの姉妹ライブラリ:** [PCMFlowOpus](https://github.com/tanakamasayuki/PCMFlowOpus)(リリース済)— 低ビットレート / 広帯域・全帯域音声向け。**PCMFlowG722**(計画中、未リリース)— G.711 と同じ 64 kbps で広帯域 HD voice。いずれも PCMFlow に `PCMSource` / `PCMSink` 経由で接続し、同じスケッチに共存できる。比較表は [README「PCMFlow コーデックファミリー」](README.ja.md#pcmflow-コーデックファミリー) を参照。圧縮率より**コーデック自体のフットプリント極小**を優先したい**リアルタイム双方向音声**(VoIP、ESP-NOW トランシーバ、WebSocket / UDP 音声リンク)に焦点を絞る。

責務:

- 16-bit signed PCM サンプル(8 kHz mono)を 8-bit G.711 バイトに**エンコード**
- 8-bit G.711 バイトを 16-bit signed PCM サンプルに**デコード**

**μ-law(PCMU、ITU-T G.711 Annex 1、RTP payload type 0)**と **A-law(PCMA、ITU-T G.711 Annex 2、RTP payload type 8)**の両方をサポート。`begin()` で切替。

コーデックは**ステートレスなバイト単位マッピング**なので、パケット内のフレーミングはコーデック側に存在しない。チャンクサイズは運搬媒体側で決める(例: RTP 20 ms フレーム = 160 バイト @ 8 kHz)。バイト列のフレーミングは呼び出し側の責任。

このライブラリは PCMFlow パイプラインへ**インタフェースレベルで接続するだけ**。PCMFlow 本体(リングバッファ、フォーマット変換、ゲイン、出力デバイス連携)は再実装しない。

## 2. 非目標

- **WAV コンテナ(μ-law / A-law ペイロード)**(`WAVE_FORMAT_MULAW` / `WAVE_FORMAT_ALAW`)— 初版対象外。§「将来対応」参照。
- **8 kHz 以外のサンプルレート** — G.711 は 8 kHz で定義された規格。I2S DAC 出力等のリサンプリングは PCMFlow の責務。
- **ステレオ** — G.711 は規格上モノラル電話用コーデック。2 チャネル運用はインスタンスを 2 つ並べて行う。
- **音声ファイル再生 / 出力デバイス制御** — PCMFlow の責務。
- **`Stream` / ファイルシステム / ネットワークアダプタ** — PCMFlow の `ByteStream` / `ByteSink` 側の責務。
- **Jitter buffer / RTP パーサ / ネットワーク転送** — 呼び出し側の責任。PCMFlowG711 はコーデックであってスタックではない。

## 3. 主目的ユースケース:ESP-NOW トランシーバ

設計の動機となるユースケース。ESP-NOW は 1 パケット最大 250 byte ペイロード。8 kHz × 8 bit/sample = 64 kbps なので、1 フレームに音声 ~31 ms 入る。自然な選択は RTP 標準の 20 ms = 160 byte:

| フレーム長 | パケットあたり(8 kHz × 8 bit) |
|------------|-------------------------------|
| 10 ms      | 80 B  |
| 20 ms(RTP 既定)| 160 B |
| 30 ms      | 240 B |

参考に、生 8 kHz mono 16-bit PCM は 20 ms あたり 320 B なので G.711 で **2×** 圧縮。Opus(10〜15×)に比べて圧縮率は低いが、コーデック自体の CPU 消費はほぼゼロ、Flash も 2 KB 以下に収まる。電波帯域に 64 kbps の余裕があり、Opus に Flash / RAM を割きたくない場合はこちらが向く。

エンド-to-エンドのトランシーバ用サンプルは [examples/EspNowTransceiver/](examples/EspNowTransceiver/) に同梱(マイク→G.711 エンコード→ESP-NOW broadcast / ESP-NOW 受信→G.711 デコード→I2S DAC、すべて 1 スケッチに収める)。

## 4. 公開 API(計画)

2 クラス。両方とも **N サンプル単位**で動作(N は呼び出し側が選ぶ、コーデック側にフレーミングはない)。PCM 1 サンプル ↔ G.711 1 バイト。

### 4.1 `G711Variant`

```cpp
enum class G711Variant : uint8_t
{
    MuLaw,   // ITU-T G.711 Annex 1; PCMU; RTP payload type 0; 北米・日本
    ALaw,    // ITU-T G.711 Annex 2; PCMA; RTP payload type 8; 欧州
};
```

### 4.2 `G711Encoder` — `PCMSink` を実装

16-bit signed PCM を受け、G.711 バイトを出力する。

```cpp
G711Encoder enc;
enc.begin({8000, 1, 16}, G711Variant::MuLaw);

uint8_t pkt[160];
const size_t n = enc.encode(pcm20ms,   // 160 samples = 20 ms @ 8 kHz
                            160,
                            pkt, sizeof(pkt));
// n == 160; pkt[0..n) を ESP-NOW / UDP / WebSocket で送信
```

公開オプション:
- `setVariant(G711Variant)` — 実行時切替(テスト用途以外ではあまり使わない)
- `variant()` — 取得

エンコーダは**設定済み variant 以外の永続状態を持たない**ので、`begin()` 後はタスク / ISR からの呼び出しも安全。

### 4.3 `G711Decoder` — `PCMSource` を実装

G.711 バイトを受け、16-bit signed PCM を出力する。

```cpp
G711Decoder dec;
dec.begin({8000, 1, 16}, G711Variant::MuLaw);

int16_t pcm[160];
const size_t frames = dec.decode(pkt, 160, pcm, 160);
// frames == 160
```

エンコーダ同様、デコーダも variant 以外の状態を持たない**ステートレス**。コーデックには **PLC を組み込まない**。パケットロス時に無音を入れる / 直前サンプルを引き伸ばす / 快音(comfort noise)を入れるかは呼び出し側で決める。G.711 は時系列予測を持たないので、簡単な PLC 戦略なら caller 側で数行で書ける(EspNowTransceiver サンプルに 1 例)。

`G711Encoder::format()` / `G711Decoder::format()` が示すのは PCM 側のフォーマット。バイト側は不透明。

### 4.4 PCMFlow パイプラインへの接続

`G711Decoder` は `PCMSource` を実装するので、`PCMFlow::setInputSource(dec)` で PCMFlow のリングバッファ / フォーマット変換 / 出力デバイス連携につながる。呼び出し側がバイト列を `decode(...)` で供給し、PCMFlow 側は `pump()` / `readFrames()` で PCM を消費する、という二段構造。

`G711Encoder` は `PCMSink` を実装。録音タスクから 16-bit PCM を `writeFrames(...)` で押し込むと、内部でバイト列に変換して、コールバックまたは指定 `ByteSink` に出す。

詳細シグネチャとエラー列挙は実装と同時に [src/](src/) のヘッダで確定する。

## 5. PCM 入出力フォーマット

- サンプルレート: **8000 Hz**(ITU-T G.711 固定レート)
- ビット深度: PCM 側 **16-bit signed**、コーデック側 **8-bit unsigned**
- チャネル数: **1 (mono)**

別レートでの出力(例: ESP32-S3 で I2S DAC を 44.1 kHz 駆動)が必要なら、PCMFlow のリサンプラに任せる。PCMFlowG711 は 8 kHz でしか動作しない。

## 6. メモリ・フットプリント目標

G.711 はテーブル変換コーデックなので、構造上フットプリントは極小。

| 項目 | 目標 |
|------|------|
| Flash(エンコーダ + デコーダ、両 variant) | ≤ 4 KB |
| Flash(デコーダのみ、両 variant、リンク時破棄) | ≤ 2 KB |
| RAM、エンコーダ永続状態 | ≤ 16 B |
| RAM、デコーダ永続状態 | ≤ 16 B |
| 呼び出しごとのスクラッチ | なし |

実装はデコード方向に 256 エントリの定数テーブル(両 variant 合計 ~1 KB Flash)、エンコード方向は ITU-T G.711 仕様のセグメント計算式(variant あたり ~200 B のコード)を使う。動的確保なし。

## 7. リポジトリ構成

```
PCMFlowG711/
├─ README.md / README.ja.md
├─ SPEC.md   / SPEC.ja.md
├─ CHANGELOG.md
├─ LICENSE
├─ library.properties        # Arduino IDE
├─ library.json              # PlatformIO
├─ keywords.txt              # Arduino IDE シンタックスハイライト
├─ src/
│  ├─ PCMFlowG711.h          # 集約ヘッダ
│  ├─ G711Encoder.h/.cpp     # PCMSink、PCM -> G.711 バイト
│  ├─ G711Decoder.h/.cpp     # PCMSource、G.711 バイト -> PCM
│  └─ pcmflowg711_version.h  # tools/bump_version.py が生成
├─ examples/
│  └─ EspNowTransceiver/     # 主目的ユースケースのサンプル
├─ tests/
│  ├─ README.md / README.ja.md
│  ├─ conftest.py
│  ├─ pyproject.toml
│  ├─ smoke/
│  ├─ g711_encoder/
│  ├─ g711_decoder/
│  ├─ roundtrip/             # encode → decode で元波形と比較
│  ├─ external_source/       # PCMFlow::setInputSource 経路の結合
│  └─ tools/gen_test_audio.py
├─ doc/
│  └─ sibling_library_brief.md
├─ tools/
│  └─ bump_version.py        # 親 PCMFlow のものをそのまま流用
└─ .github/
   └─ workflows/
      └─ release.yml         # 親 PCMFlow のものをそのまま流用
```

## 8. Vendor している上流

**なし。** G.711 のアルゴリズムは ITU-T G.711 勧告(1972 年、公開規格として自由に参照可能)に規定されている。規範的なのは数学的写像のみで、数学的アルゴリズムは著作権の対象にならない。PCMFlowG711 のエンコーダ / デコーダは規格本文から起こしており、いかなる第三者ソースも複製していない。

これは**ライセンス衛生上の意図的な選択**:vendor 上流がないので、配布するライブラリ全体が **MIT、単独著者、追加の attribution 不要**(標準 MIT 表記のみ)。`src/external/` も `LICENSE_*.md` も `UPSTREAM.lock` も存在しない。

実装時に参照した規格資料(アルゴリズム仕様のみ、コード流用なし):

- ITU-T Recommendation G.711 (11/1988), "Pulse code modulation (PCM) of voice frequencies"

## 9. リリースフロー

PCMFlowG711 は上流追随の自動化レベルとして **L0 — 上流追随なし** を採用する(追随する上流が存在しないため)。PCMFlowG711 自体のリリースは完全自動で、親 PCMFlow と同じ。

### 最終リリースのタグ付け

バージョン bump と Arduino Library Manager 用 tag は `tools/bump_version.py`(親 PCMFlow から流用)で生成し、[`.github/workflows/release.yml`](.github/workflows/release.yml)(これも親から移植)が駆動する。`version=` と CHANGELOG の `Unreleased` 節は同時に動く、親と同じ流儀。メンテナが `workflow_dispatch` で起動する。

`upstream-check.yml` は**存在しない** — ITU-T 勧告は 1988 年以来安定で、仮に改訂があってもライブラリ側のアクションは伴わないため。

## 10. テスト

親 PCMFlow と同じ規約:

- pytest-embedded + Arduino CLI バックエンド
- 2 プロファイル:`lang-ship:host`(ロジック・大サイズ fixture)と `esp32:esp32:esp32`(実機検証)
- 機能ごとのディレクトリに `<feature>.ino` / `sketch.yaml` / `test_<feature>.py` / `input/` fixture
- アサーション形式は `EXPECT_TRUE` / `EXPECT_EQ` / `EXPECT_NEAR` マクロ、Serial 経由の `TEST done N/M` プロトコル

G.711 固有のテスト設計:

| テストディレクトリ | 検査対象 | 戦略 |
|---|---|---|
| `g711_encoder/` | 両 variant について、エンコーダ出力が規格定義の写像と一致するか | 既知 PCM ランプを入力し、`tools/gen_test_audio.py` で独立生成した参照テーブルとバイト単位で比較 |
| `g711_decoder/` | 両 variant について、デコーダ出力が規格定義の写像と一致するか | 256 通りすべてのバイト値を入力し、参照デコードテーブルと比較 |
| `roundtrip/` | encode → decode が**符号語アライン入力に対しては idempotent**、それ以外も量子化ステップ許容差以内 | RMS 誤差が理論最悪量子化雑音以下。符号・ゼロクロスが保存される |
| `external_source/` | `G711Decoder` が `PCMFlow::setInputSource()` 経由で機能するか | 親 FLAC テストと同じハーネス |

G.711 は**テーブル変換型ロッシー**なので、エンコード / デコードのテーブルそのものを**ビット精確**に独立参照と比較できる(Opus と違って ±許容差ではなく bit-exact)。テスト基準は Opus より厳しい。

## 11. バージョニング

SemVer (`major.minor.patch`) を `library.properties`、`library.json`、`src/pcmflowg711_version.h` で同期管理。PCMFlow のバージョンとは**独立**。

## 12. 将来対応 {#将来対応}

忘れないようここに集約。v0.1.x には含めない:

- **WAV コンテナ(μ-law / A-law ペイロード)**(読み書き)。デスクトップ相互運用(Audacity / ffmpeg / VoIP 録音再生)用。親 PCMFlow `WAVDecoder` が非 PCM ペイロードのフックを持つ形で実装するか、本リポに `G711WAVDecoder` として実装する。具体的な要望が出てから着手。
- **G.711 Appendix I / II PLC** — ITU-T 規定のパケットロス補償。jitter のある RTP 向け。8 kHz では「最終サンプル保持」「線形フェード」で十分なケースが多い。Appendix I は ~1 KB の状態と ~20 行のコードで実装可能、追加価値はあるが v0.1 では優先しない。
- **G.711.0 ロスレス圧縮器** — 別の(後発の)ITU-T 勧告。G.711 パケットをロスレスで ~50 % 縮める。対象外。

## 13. ライセンス

PCMFlowG711: **MIT** ([LICENSE](LICENSE))。Vendor している第三者コードなし、追加の attribution 不要。
