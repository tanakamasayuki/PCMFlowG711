# Tests

> 日本語版: [README.ja.md](README.ja.md)

Automated test suite for PCMFlowG711. Mirrors the conventions of the parent [PCMFlow test suite](https://github.com/tanakamasayuki/PCMFlow/tree/main/tests):

- [pytest-embedded](https://docs.espressif.com/projects/pytest-embedded/en/latest/) + Arduino CLI backend.
- Two profiles: `lang-ship:host` (logic verification, large fixtures, fast CI) and `esp32:esp32:esp32` (real hardware verification, footprint measurement).
- Per-feature subdirectory containing `<feature>.ino`, `sketch.yaml`, `test_<feature>.py`, and an `input/` directory of fixtures.
- Assertions use the `EXPECT_TRUE` / `EXPECT_EQ` / `EXPECT_NEAR` macros and the `TEST done N/M` Serial protocol.

## Status

**Scaffolding.** Smoke test is in place; per-codec test directories contain placeholder sketches that the Python side currently asserts only for "TEST start" + "TEST done". The real assertions land when the encoder / decoder implementations do.

## Directory layout

- `smoke/` — Template smoke test (host profile). Verifies the test infrastructure itself.
- `g711_encoder/` — Unit tests for `G711Encoder` (input PCM → exact G.711 byte mapping, both variants).
- `g711_decoder/` — Unit tests for `G711Decoder` (all 256 byte values → expected PCM, both variants).
- `roundtrip/` — End-to-end encode → decode tests. Verifies PCM ≈ PCM' within G.711's quantization-step tolerance, and bit-exact for codeword-aligned inputs.
- `external_source/` — Integration test for `G711Decoder` plugged into PCMFlow's `setInputSource()` pipeline.
- `tools/gen_test_audio.py` — Generates reference encode / decode tables and embed-able `.h` fixtures under each test's `input/` directory. Uses only the Python standard library — no external codec dependency required, since the reference mapping is defined by the ITU-T specification and reproduced independently here.

## G.711-specific test design

G.711 is a **table-lookup lossy** codec, so encode / decode is fully deterministic and the test suite can be **bit-exact** against an independent reference (unlike Opus, which only allows ±tolerance):

| Test dir | What we check | Tolerance |
|----------|---------------|-----------|
| `g711_encoder/` | Each output byte for a known PCM input matches the reference table | exact, bit-for-bit |
| `g711_decoder/` | Each decoded PCM sample for all 256 input bytes matches the reference table | exact, bit-for-bit |
| `roundtrip/` | encode → decode reproduces input within the quantization step of the relevant segment | ≤ codeword step (deterministic), sign preserved |
| `external_source/` | `G711Decoder` plumbs through `PCMFlow::setInputSource()` | exact byte → PCM round-trip |

## Running

```sh
# host (default)
uv run --env-file .env pytest

# real ESP32
uv run --env-file .env pytest --profile=esp32

# single test
uv run --env-file .env pytest g711_decoder/
```

See the [parent PCMFlow tests README](https://github.com/tanakamasayuki/PCMFlow/tree/main/tests#prerequisites) for prerequisites (uv, arduino-cli, board cores).
