# PCMFlowG711

> 日本語版: [README.ja.md](README.ja.md)

Optional **G.711** (μ-law / A-law) codec add-on for [PCMFlow](https://github.com/tanakamasayuki/PCMFlow), aimed at **real-time two-way voice over packet radio / network** — VoIP, ESP-NOW transceivers, WebSocket / UDP voice links.

A **clean-room MIT** implementation of ITU-T G.711. No third-party source code is vendored; the entire library is single-author MIT, so embedding it does not impose any attribution beyond the standard MIT notice. Flash footprint is well under 4 KB for the full encoder + decoder, both variants.

See [SPEC.md](SPEC.md) for the full specification.

---

## Status

**Pre-release / scaffolding.** Repository layout, documentation, release tooling, and the public API surface are in place. The encoder / decoder implementations are not yet wired in. See [CHANGELOG.md](CHANGELOG.md).

---

## What's inside

| Class | Direction | Carrier | Interface |
|-------|-----------|---------|-----------|
| `G711Encoder` | PCM → G.711 byte | raw bytes (RTP / ESP-NOW / UDP / WebSocket) | `PCMSink` |
| `G711Decoder` | G.711 byte → PCM | raw bytes | `PCMSource` |

One PCM sample maps to exactly one G.711 byte; there is no internal framing, so the carrier chooses the chunk size (typically 160 bytes = 20 ms at 8 kHz, matching RTP). Both **μ-law (PCMU, RTP payload type 0)** and **A-law (PCMA, RTP payload type 8)** are supported, selectable at `begin()` time.

WAV-container support (`WAVE_FORMAT_MULAW` / `WAVE_FORMAT_ALAW` files) is intentionally **out of scope for the initial release** — see [SPEC §"Deferred features"](SPEC.md#deferred-features).

---

## PCMFlow codec family

PCMFlowG711 is one member of a family of optional codec add-ons for PCMFlow. Pick whichever matches your bandwidth / footprint / quality budget:

| | **PCMFlowG711** (this lib) | [PCMFlowG722](https://github.com/tanakamasayuki/PCMFlowG722) (planned) | [PCMFlowOpus](https://github.com/tanakamasayuki/PCMFlowOpus) |
|---|---|---|---|
| Audio band | narrowband (8 kHz / ≤ 3.4 kHz) | **wideband (16 kHz / ≤ 7 kHz)** | narrow / wide / fullband (8–48 kHz) |
| Bitrate (voice) | 64 kbps fixed | 48 / 56 / 64 kbps | 16–32 kbps typical |
| Compression vs raw 16-bit PCM | 2× | 4× | 10–15× |
| Codec flash footprint | **< 4 KB** | ~10 KB (est.) | ~150–180 KB |
| Codec CPU | negligible | low | non-trivial on M0/M3-class MCUs |
| Patent / license complexity | none (1972 standard, expired) | none (1988 standard, expired) | royalty-free patent grant, BSD-3-Clause source |
| Quality | toll-grade telephony | HD voice (wideband telephony) | wideband / fullband, far better |

Pick G.711 when:
- You have 64 kbps of radio / network budget to spend.
- You want the smallest possible codec footprint (e.g. coexisting with a large application, or running on a memory-tight board).
- You want a clean, single-license MIT codebase with no third-party attribution.

Pick [G.722 (PCMFlowG722)](https://github.com/tanakamasayuki/PCMFlowG722) — **planned sibling library, not yet released** — when you want **HD-voice wideband audio at the same 64 kbps** as G.711, with only a moderate footprint increase. G.722 sits in the natural middle of the family between G.711 and Opus.

Pick [Opus (PCMFlowOpus)](https://github.com/tanakamasayuki/PCMFlowOpus) when you need the bandwidth savings or fullband audio quality.

---

## Headline use case — ESP-NOW transceiver

ESP-NOW carries up to 250 byte payloads. A 20 ms G.711 voice frame at 8 kHz × 8 bit is exactly 160 byte → comfortably fits with room for headers. Raw 8 kHz mono 16-bit PCM at the same 20 ms is 320 byte — G.711 compresses **2×**.

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

// Called from the ESP-NOW recv callback: feed bytes into decoder
void onEspNowRecv(const uint8_t *mac, const uint8_t *data, int len) {
    dec.decode(data, len, /*pcm_out=*/nullptr, /*max_frames=*/0);
    // PCMFlow will pump() and play the decoded PCM on the I2S/DAC side
}
```

End-to-end transceiver sketch (mic ↔ ESP-NOW ↔ DAC) lives in [examples/EspNowTransceiver/](examples/EspNowTransceiver/) (placeholder for now).

---

## Dependencies

- **[PCMFlow](https://github.com/tanakamasayuki/PCMFlow)** — provides `PCMSource`, `PCMSink`, `ByteStream`, `ByteSink`, and the playback pipeline. Declared via `depends=PCMFlow` in `library.properties`.

That's it. No vendored codec, no external libraries.

---

## Target platforms

Same posture as PCMFlow: `architectures=*` in `library.properties`, with the same realistic constraints — 32-bit MCU, even modest SRAM, a few KB of free flash.

Practical targets: ESP32 / ESP32-S3 / ESP32-C3 / ESP32-C6 / ESP32-P4, RP2040 / RP2350, Teensy 4.x, STM32 F4+, nRF52. G.711 is light enough that AVR (Uno / Mega / Nano) and SAMD21-class boards are **also viable** if you have RAM left over for PCMFlow itself.

Provisional footprint (encoder + decoder, both variants): **Flash < 4 KB, RAM ≤ 32 B per instance**. Decoder-only builds drop the flash side further when the encoder is unreferenced and the linker discards it. Detailed targets live in [SPEC.md §6](SPEC.md).

---

## License

PCMFlowG711: **MIT** ([LICENSE](LICENSE)).

The library is a clean-room implementation of the ITU-T G.711 algorithm (a public 1972 standard whose mathematical mapping is not copyrightable). **No third-party source code is vendored**, so there is no `src/external/`, no additional license file, and no attribution required beyond the standard MIT notice.

---

## Tests

```sh
cd tests
uv run --env-file .env pytest                  # host (default)
uv run --env-file .env pytest --profile=esp32  # real ESP32 hardware
```

See [tests/README.md](tests/README.md). Same conventions as the parent PCMFlow test suite (pytest-embedded + Arduino CLI, `lang-ship:host` + `esp32:esp32:esp32` profiles).
