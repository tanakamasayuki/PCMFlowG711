# Examples

Two ready-to-run sketches targeting M5Stack Core2 (`esp32:esp32:m5stack_core2`). Both default to **μ-law**; flip `kVariant` to `G711Variant::ALaw` at the top of the sketch to switch.

| Sketch | Hardware | What it shows |
|---|---|---|
| **[MicLoopback](MicLoopback/)** | 1 × Core2 | Mic → G711Encoder → G711Decoder → PCMFlow → Speaker, all on one board. Use this first to verify the codec + audio chain before trying the radio version. |
| **[EspNowTransceiver](EspNowTransceiver/)** | 2 × Core2 (same channel) | Headline use case. Flash the same firmware on both boards. Hold A on one to broadcast 20 ms / 160 byte μ-law voice frames over ESP-NOW; the other board plays them. |

## Build & flash (M5Stack Core2)

Sketches ship `sketch.yaml` with an `esp32:esp32:m5stack_core2` profile, so:

```sh
cd examples/MicLoopback        # or EspNowTransceiver
arduino-cli compile --profile m5stack_core2
arduino-cli upload  --profile m5stack_core2 -p /dev/ttyACM0
```

The profile pulls in PCMFlow + M5Unified automatically; PCMFlowG711 is taken from the parent directory via `dir: ../../` so a registered version is not required for local development.

## Why these two

`MicLoopback` is the minimum reproduction of "the codec works in a real audio pipeline" — useful for triaging issues: if the loopback is silent or distorted, that's a codec / mic / speaker problem, not an ESP-NOW problem. Once it works, `EspNowTransceiver` adds the radio on top using exactly the same encode / decode glue.

## Adapting to other boards

The codec itself has no platform dependency — `G711Encoder` / `G711Decoder` compile anywhere `<Arduino.h>` does. Only the **audio I/O and ESP-NOW glue** in these sketches are Core2-specific (M5.Mic / M5.Speaker / esp_now.h). For another ESP32 board with a different mic / DAC, swap the M5 calls for your I2S setup and the rest carries over verbatim.

For non-ESP32 targets the codec still works; you lose ESP-NOW but UDP / WebSocket / serial-tethered carriers are all valid replacements.
