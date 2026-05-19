// EspNowTransceiver — placeholder sketch.
//
// Showcase sketch for the headline PCMFlowG711 use case:
//   mic  → G711Encoder (8 kHz mu-law / A-law) → ESP-NOW broadcast
//   ESP-NOW receive → G711Decoder → PCMFlow → I2S DAC
//
// All in one binary, so two boards flashed with the same firmware can
// half-duplex talk to each other. The G.711 path is well below 1 KB of
// flash and adds essentially zero CPU on an ESP32-class MCU, so most
// of the work is in the audio I/O wiring around it.
//
// STATUS: not implemented yet. Will be filled in once G711Encoder /
// G711Decoder are functional. The README references this directory as
// the headline example, so it is kept in the scaffolding even before
// the implementation exists.

#include <PCMFlow.h>
#include <PCMFlowG711.h>

void setup()
{
    Serial.begin(115200);
    Serial.println("EspNowTransceiver — placeholder.");
    // TODO: implement once G711Encoder/G711Decoder are available.
}

void loop()
{
    delay(1);
}
