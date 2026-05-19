# Examples

Sketches under this directory:

- **EspNowTransceiver** — Headline use case for PCMFlowG711. A single firmware that records from the mic, G.711-encodes 20 ms voice frames (160 bytes at 8 kHz × 8 bit), and broadcasts them over ESP-NOW; on the receive side it decodes incoming G.711 bytes and pipes them through PCMFlow to an I2S DAC. Two boards flashed with the same firmware can half-duplex talk to each other.

  **Status: placeholder.** Will be filled in once `G711Encoder` / `G711Decoder` are functional.

More examples will be added as the codec and surrounding tooling come online (RTP-payload-type-0 bridge, WebSocket bridge to a browser, mu-law WAV playback shim, etc.).
