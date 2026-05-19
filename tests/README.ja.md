# Tests

> English: [README.md](README.md)

PCMFlowG711 の自動テストスイート。親 [PCMFlow テストスイート](https://github.com/tanakamasayuki/PCMFlow/tree/main/tests) と同じ規約:

- [pytest-embedded](https://docs.espressif.com/projects/pytest-embedded/en/latest/) + Arduino CLI バックエンド
- 2 プロファイル: `lang-ship:host`(ロジック検証・大サイズ fixture・高速 CI)と `esp32:esp32:esp32`(実機検証・フットプリント計測)
- 機能ごとのサブディレクトリに `<feature>.ino` / `sketch.yaml` / `test_<feature>.py` / `input/` fixture
- アサーションは `EXPECT_TRUE` / `EXPECT_EQ` / `EXPECT_NEAR` マクロ + Serial 経由の `TEST done N/M` プロトコル

## ディレクトリ構成

- `smoke/` — テンプレート smoke テスト(host プロファイル)。テスト基盤そのものを検証
- `g711_encoder/` — `G711Encoder` 単体テスト(入力 PCM → G.711 バイトの規格写像、両 variant)
- `g711_decoder/` — `G711Decoder` 単体テスト(256 通りの入力バイト → 期待 PCM、両 variant)
- `roundtrip/` — end-to-end encode → decode。量子化ステップ許容差以内、符号語アライン入力は bit-exact
- `external_source/` — `G711Decoder` を `PCMFlow::setInputSource()` 経由で繋ぐ結合テスト
- `tools/gen_test_audio.py` — エンコード / デコードの参照テーブルと埋め込み可能な `.h` を `input/` 配下に生成。ITU-T 規格の数式を Python 標準ライブラリのみで独立再現するため、外部コーデック依存なし

## G.711 固有のテスト設計

G.711 は**テーブル変換型ロッシー**なので、encode / decode は完全に決定論的。Opus と違いテストは**独立参照に対して bit-exact**:

| テストディレクトリ | 検証内容 | 許容差 |
|---|---|---|
| `g711_encoder/` | 既知 PCM 入力に対し、出力バイトが参照テーブルと一致するか | exact、bit-for-bit |
| `g711_decoder/` | 256 通り全入力バイトに対し、PCM 出力が参照テーブルと一致するか | exact、bit-for-bit |
| `roundtrip/` | encode → decode が当該セグメントの量子化ステップ以内に収まる | 符号語ステップ以下(決定論的)、符号保存 |
| `external_source/` | `G711Decoder` が `PCMFlow::setInputSource()` 経由で機能するか | byte → PCM ラウンドトリップ exact |

## 実行

```sh
# host (デフォルト)
uv run --env-file .env pytest

# 実機 ESP32
uv run --env-file .env pytest --profile=esp32

# 単一テスト
uv run --env-file .env pytest g711_decoder/
```

前提(uv、arduino-cli、各ボードコア)については [親 PCMFlow tests README](https://github.com/tanakamasayuki/PCMFlow/tree/main/tests#prerequisites) を参照。
