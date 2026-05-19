# PCMFlowG711 Specification

> 日本語版: [SPEC.ja.md](SPEC.ja.md)

## 1. Scope

**PCMFlowG711** is an optional G.711 codec add-on for [PCMFlow](https://github.com/tanakamasayuki/PCMFlow), focused on **real-time two-way voice / packet-radio use cases**

> **Sibling libraries in the same family:** [PCMFlowOpus](https://github.com/tanakamasayuki/PCMFlowOpus) (released) for low-bitrate / wide- and fullband audio; **PCMFlowG722** (planned, not yet released) for wideband HD voice at the same 64 kbps budget as G.711. All three plug into PCMFlow through `PCMSource` / `PCMSink` and can coexist in the same sketch. See [README "PCMFlow codec family"](README.md#pcmflow-codec-family) for the trade-off table. (VoIP, ESP-NOW transceivers, WebSocket / UDP voice links) where the smallest possible codec footprint is preferred over compression ratio.

Responsibility:

- **Encode** 16-bit signed PCM samples (8 kHz mono) into 8-bit G.711 bytes.
- **Decode** 8-bit G.711 bytes back into 16-bit signed PCM samples.

Both **μ-law (PCMU, ITU-T G.711 Annex 1, RTP payload type 0)** and **A-law (PCMA, ITU-T G.711 Annex 2, RTP payload type 8)** are supported, selectable at `begin()` time.

The codec is a **stateless byte-by-byte mapping**, so packets have no framing within the codec itself — the carrier defines the chunk size (e.g. 160 bytes for a 20 ms RTP frame at 8 kHz). The byte stream's framing is the caller's responsibility.

The library plugs into PCMFlow's pipeline at the interface level only. It does not reimplement PCMFlow internals (ring buffer, format conversion, gain, output handoff are owned by PCMFlow).

## 2. Non-goals

- **WAV container with μ-law / A-law payload** (`WAVE_FORMAT_MULAW` / `WAVE_FORMAT_ALAW`) — out of scope for the initial release. See §"Deferred features".
- **Sample rates other than 8 kHz** — G.711 is defined only at 8 kHz. Resampling for I2S DAC output is owned by PCMFlow.
- **Stereo** — G.711 is a mono telephony codec by specification. Two-channel use is achieved by running two independent codec instances.
- **Audio file playback / device output** — owned by PCMFlow.
- **`Stream` / file system / network adapters** — owned by PCMFlow's `ByteStream` / `ByteSink`.
- **Jitter buffering / RTP parsing / network transport** — caller's responsibility. PCMFlowG711 is a codec, not a stack.

## 3. Primary use case: ESP-NOW transceiver

The motivating use case. ESP-NOW carries up to 250 byte payloads. At 8 kHz / 8 bits per sample (64 kbps), one ESP-NOW frame holds up to ~31 ms of voice; the natural choice is the RTP-standard 20 ms = 160 byte chunk:

| Frame duration | Bytes per packet (8 kHz × 8 bit) |
|----------------|----------------------------------|
| 10 ms          | 80 B  |
| 20 ms (RTP default) | 160 B |
| 30 ms          | 240 B |

Compare to raw 8 kHz mono 16-bit PCM: 320 B per 20 ms. G.711 compresses **2×** — far less than Opus (10–15×) but the codec itself adds essentially zero CPU and well under 2 KB of flash, making it the right choice when the radio budget allows 64 kbps and you do not want to spend flash / RAM on Opus.

An end-to-end transceiver sketch lives in [examples/EspNowTransceiver/](examples/EspNowTransceiver/) (mic → G.711 encode → ESP-NOW broadcast / ESP-NOW receive → G.711 decode → I2S DAC, all in one sketch).

## 4. Public API (planned)

Two classes; both work on **N samples at a time** (N chosen by the caller — there is no internal framing). One PCM sample maps to exactly one G.711 byte.

### 4.1 `G711Variant`

```cpp
enum class G711Variant : uint8_t
{
    MuLaw,   // ITU-T G.711 Annex 1; PCMU; RTP payload type 0; common in NA / JP
    ALaw,    // ITU-T G.711 Annex 2; PCMA; RTP payload type 8; common in EU
};
```

### 4.2 `G711Encoder` — implements `PCMSink`

Accepts 16-bit signed PCM and emits G.711 bytes.

```cpp
G711Encoder enc;
enc.begin({8000, 1, 16}, G711Variant::MuLaw);

uint8_t pkt[160];
const size_t n = enc.encode(pcm20ms,    // 160 samples = 20 ms at 8 kHz
                            160,
                            pkt, sizeof(pkt));
// n == 160; send pkt[0..n) over ESP-NOW / UDP / WebSocket
```

Options:
- `setVariant(G711Variant)` — runtime swap (rarely useful; mostly for tests).
- `variant()` — query.

The encoder has **no persistent state beyond the configured variant** — it is safe to call from any task / ISR-deferred context once `begin()` has returned.

### 4.3 `G711Decoder` — implements `PCMSource`

Accepts G.711 bytes and emits 16-bit signed PCM.

```cpp
G711Decoder dec;
dec.begin({8000, 1, 16}, G711Variant::MuLaw);

int16_t pcm[160];
const size_t frames = dec.decode(pkt, 160, pcm, 160);
// frames == 160
```

Like the encoder, the decoder is **stateless** beyond the variant. There is **no PLC** built into the codec — if a packet is lost, the caller decides whether to feed silence, hold the last sample, or insert comfort noise. (G.711 has no temporal prediction, so simple PLC strategies are trivial to implement caller-side; see the EspNowTransceiver example for one pattern.)

Both `G711Encoder::format()` and `G711Decoder::format()` describe the PCM side; the byte side is opaque.

### 4.4 PCMFlow pipeline integration

`G711Decoder` implements `PCMSource`, so it plugs into PCMFlow's ring buffer / format converter / output device chain via `PCMFlow::setInputSource(dec)`. The caller feeds bytes into the decoder (`decode(...)`); PCMFlow consumes the resulting PCM through `pump()` / `readFrames()` as usual.

`G711Encoder` implements `PCMSink`. Source PCM (e.g. from a mic recording task) is pushed via `writeFrames(...)`; the encoder converts in-place and emits the bytes to the caller-supplied callback or `ByteSink`.

Detailed signatures and error enums are finalized in the headers under [src/](src/) once implemented.

## 5. PCM I/O format

- Sample rate: **8000 Hz** (ITU-T G.711 fixed rate).
- Bit depth: **16-bit signed** on the PCM side, **8-bit unsigned** on the codec side.
- Channels: **1 (mono)**.

If the application needs a different output sample rate (e.g. ESP32-S3 I2S DAC running at 44.1 kHz), PCMFlow's resampler handles the conversion. PCMFlowG711 only operates at 8 kHz.

## 6. Memory & footprint targets

G.711 is a table-lookup codec, so the footprint is tiny by construction.

| Item | Target |
|------|--------|
| Flash (encoder + decoder, both variants) | ≤ 4 KB |
| Flash (decoder only, both variants, link-time discarded encoder) | ≤ 2 KB |
| RAM, persistent encoder state | ≤ 16 B |
| RAM, persistent decoder state (direct decode only) | ≤ 16 B |
| RAM, persistent decoder state (with PCMSource queue for `setInputSource`) | ~320 B |
| Per-call scratch | none |

The implementation uses precomputed 256-entry lookup tables for the decode direction (~1 KB Flash for both variants combined) and the standard segment-based encode formulas from the ITU-T G.711 specification (≤ ~200 B of code per variant). No dynamic allocation.

## 7. Repository layout

```
PCMFlowG711/
├─ README.md / README.ja.md
├─ SPEC.md   / SPEC.ja.md
├─ CHANGELOG.md
├─ LICENSE
├─ library.properties        # Arduino IDE
├─ library.json              # PlatformIO
├─ keywords.txt              # Arduino IDE syntax highlight
├─ src/
│  ├─ PCMFlowG711.h          # umbrella header
│  ├─ G711Encoder.h/.cpp     # PCMSink, PCM -> G.711 byte
│  ├─ G711Decoder.h/.cpp     # PCMSource, G.711 byte -> PCM
│  └─ pcmflowg711_version.h  # auto-generated by tools/bump_version.py
├─ examples/
│  └─ EspNowTransceiver/     # showcase sketch (mic↔ESP-NOW↔DAC)
├─ tests/
│  ├─ README.md / README.ja.md
│  ├─ conftest.py
│  ├─ pyproject.toml
│  ├─ smoke/
│  ├─ g711_encoder/
│  ├─ g711_decoder/
│  ├─ roundtrip/             # encode → decode → compare against original
│  ├─ external_source/       # PCMFlow::setInputSource integration
│  └─ tools/gen_test_audio.py
├─ doc/
│  └─ sibling_library_brief.md
├─ tools/
│  └─ bump_version.py        # mirrors parent PCMFlow's tooling
└─ .github/
   └─ workflows/
      └─ release.yml         # mirrors parent PCMFlow's tooling
```

## 8. Vendored upstream

**None.** The G.711 algorithm is specified in the ITU-T G.711 recommendation (1972, freely published as a standard); only the mathematical mapping is normative and mathematical algorithms are not copyrightable. PCMFlowG711's encoder / decoder are written from the specification, without reproducing any third-party source code.

This is a deliberate license-hygiene choice: with no vendored upstream, the entire shipped library is **MIT, single-author, no third-party attribution required** beyond the standard MIT notice. There is no `src/external/`, no `LICENSE_*.md`, and no `UPSTREAM.lock`.

Reference sources consulted during implementation (algorithm specification only, no code reuse):

- ITU-T Recommendation G.711 (11/1988), "Pulse code modulation (PCM) of voice frequencies".

## 9. Release workflow

PCMFlowG711 adopts **automation level L0 — no upstream tracking**, because there is no upstream to track. The release of PCMFlowG711 itself is fully automated, identical to parent PCMFlow.

### Final release tagging

Version bumps and the Arduino Library Manager tag are produced by `tools/bump_version.py` (copied verbatim from parent PCMFlow), driven by [`.github/workflows/release.yml`](.github/workflows/release.yml) (also mirrored from parent). PCMFlowG711's `version=` and `Unreleased` CHANGELOG section move together, identically to PCMFlow. The maintainer triggers it via `workflow_dispatch` after merging any change worth releasing.

There is no `upstream-check.yml` — the ITU-T recommendation has been stable since 1988 and no library-side action would follow from a (hypothetical) revision.

## 10. Testing

Same conventions as parent PCMFlow:

- pytest-embedded + Arduino CLI backend.
- Two profiles: `lang-ship:host` (logic, large fixtures) and `esp32:esp32:esp32` (real hardware verification).
- Per-feature test directory with `<feature>.ino`, `sketch.yaml`, `test_<feature>.py`, and `input/` fixtures.
- Assertions use the `EXPECT_TRUE / EXPECT_EQ / EXPECT_NEAR` macro family and the `TEST done N/M` Serial protocol.

G.711-specific test design:

| Test dir | Subject | Strategy |
|----------|---------|----------|
| `g711_encoder/` | encoder output matches the canonical mapping for both variants | feed a known PCM ramp; assert each byte against an independently-computed reference table generated by `tools/gen_test_audio.py` |
| `g711_decoder/` | decoder output matches the canonical mapping for both variants | feed all 256 possible byte values; assert PCM matches the reference decode table |
| `roundtrip/` | encode → decode is **idempotent for codeword-aligned inputs** and within the quantization-step tolerance otherwise | RMS error stays below the theoretical worst-case quantization noise; sign and zero-crossings preserved |
| `external_source/` | `G711Decoder` works as a `PCMSource` plugged into `PCMFlow::setInputSource()` | same harness as the parent FLAC test |

G.711 is a **table-lookup lossy** codec, so the encode/decode tables can be asserted **bit-exact** against an independent reference (unlike Opus, which only allows ±tolerance). This makes the test suite much stricter than for Opus.

## 11. Versioning

SemVer (`major.minor.patch`) maintained in `library.properties`, `library.json`, and `src/pcmflowg711_version.h`. **Independent of** the PCMFlow version.

## 12. Deferred features {#deferred-features}

Captured here so they aren't lost; not in v0.1.x:

- **WAV container with μ-law / A-law payload** (read and write). Useful for desktop interop (Audacity / ffmpeg / VoIP recording playback). Implemented as a thin shim around the existing parent PCMFlow `WAVDecoder` once the parent grows a non-PCM payload hook, or as a standalone `G711WAVDecoder` here. Defer until a concrete user request appears.
- **G.711 Appendix I / II PLC** — ITU-T-specified packet-loss concealment. Useful for jittery RTP. Most callers find that "last-sample hold" or "linear fade to silence" is sufficient at 8 kHz; Appendix I requires ~1 KB of state and ~20 lines of code, so it would be a worthwhile add but is not v0.1 priority.
- **G.711.0 lossless compressor** — a different (later) ITU-T recommendation; ~50 % size reduction on top of G.711 packets, lossless. Out of scope.

## 13. License

PCMFlowG711: **MIT** ([LICENSE](LICENSE)). No vendored third-party code; no additional attribution required.
