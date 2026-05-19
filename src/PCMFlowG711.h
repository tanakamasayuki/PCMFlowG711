#ifndef PCMFLOWG711_H
#define PCMFLOWG711_H

// Umbrella header for PCMFlowG711.
//
// Including this single header gives the user the public API surface of
// the optional G.711 codec add-on for PCMFlow:
//
//   - G711Encoder : 16-bit PCM sample -> 8-bit G.711 byte (implements PCMSink)
//   - G711Decoder : 8-bit G.711 byte  -> 16-bit PCM sample (implements PCMSource)
//
// Both mu-law (PCMU, RTP payload type 0) and A-law (PCMA, RTP payload
// type 8) are supported, selectable at begin() time via G711Variant.
//
// WAV-container support (WAVE_FORMAT_MULAW / WAVE_FORMAT_ALAW) is not
// included in the v0.1.x surface; see SPEC.md "Deferred features".

#include "pcmflowg711_version.h"
#include "G711Encoder.h"
#include "G711Decoder.h"

#endif // PCMFLOWG711_H
