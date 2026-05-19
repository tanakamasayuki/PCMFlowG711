# Changelog / 変更履歴

## Unreleased
- (EN) Repository scaffolding: README / SPEC / library metadata / src & tests skeleton. No encoder or decoder implementation yet.
- (EN) Clean-room MIT design: no third-party vendored code; ITU-T G.711 algorithm implemented from the published specification.
- (EN) Release workflow copied verbatim from parent PCMFlow. No upstream-check workflow — there is no upstream to track.
- (EN) Tests scaffolding mirrors PCMFlowOpus conventions; `tests/tools/gen_test_audio.py` ships reference μ-law / A-law mappings in pure stdlib Python (no ffmpeg dependency).
- (JA) リポジトリ雛形整備: README / SPEC / ライブラリメタデータ / src・tests スケルトン。エンコーダ・デコーダ本体は未実装。
- (JA) クリーンルーム MIT 設計: 第三者ソースの vendor なし。ITU-T G.711 アルゴリズムは公開規格から独立実装。
- (JA) リリースワークフローを親 PCMFlow からそのまま流用。`upstream-check` ワークフローは設置しない(追随する上流が存在しないため)。
- (JA) tests 雛形は PCMFlowOpus 規約に準拠。`tests/tools/gen_test_audio.py` は Python 標準ライブラリのみで μ-law / A-law 参照テーブルを生成(ffmpeg 等の依存なし)。
- (EN) Documented the planned `PCMFlowG722` sibling library alongside the existing `PCMFlowOpus` reference in README / SPEC.
- (JA) 計画中の姉妹ライブラリ `PCMFlowG722` を README / SPEC に既存の `PCMFlowOpus` と並べて記載。
