#ifndef PCMFLOWG711_G711VARIANT_H
#define PCMFLOWG711_G711VARIANT_H

#include <stdint.h>

// G.711 has two variants standardized by ITU-T. They share the same
// frame structure (1 byte per 16-bit PCM sample at 8 kHz mono) but use
// different logarithmic companding curves.
//
//   - MuLaw : ITU-T G.711 Annex 1; mu = 255; common in North America / Japan.
//             Carried as RTP payload type 0 (PCMU).
//   - ALaw  : ITU-T G.711 Annex 2; A = 87.6; common in Europe / rest of world.
//             Carried as RTP payload type 8 (PCMA).
//
// The two are NOT interoperable bit-for-bit. Sender and receiver must
// agree on variant out of band (e.g. via RTP payload type, ESP-NOW
// pairing handshake, sketch-side constant).
enum class G711Variant : uint8_t
{
    MuLaw,
    ALaw,
};

#endif // PCMFLOWG711_G711VARIANT_H
