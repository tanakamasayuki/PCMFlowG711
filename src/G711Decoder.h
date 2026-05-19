#ifndef PCMFLOWG711_G711DECODER_H
#define PCMFLOWG711_G711DECODER_H

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

#include "PCMFormat.h"
#include "PCMSource.h"
#include "G711Variant.h"

// PCMFlowG711 :: G711Decoder
//
// Decodes 8-bit G.711 bytes (mu-law or A-law) into 16-bit signed PCM
// samples (8 kHz mono). One G.711 byte maps to exactly one PCM sample.
//
// The decoder is stateless beyond the configured variant. Packet loss
// concealment (PLC) is NOT built in — when a packet is missing, the
// caller decides whether to feed silence, hold the last sample, or
// insert comfort noise. See the EspNowTransceiver example for one
// minimal pattern.
//
// STATUS: header skeleton only. Implementation is forthcoming.
//         Signatures below are SPEC §4.3 sketches.

class G711Decoder : public PCMSource
{
public:
    enum class Error : uint8_t
    {
        None,
        NotReady,
        InvalidFormat,  // sample rate != 8000, channels != 1, or bits != 16
        BufferTooSmall, // PCM capacity < packetBytes
    };

    G711Decoder() = default;
    ~G711Decoder() = default;

    G711Decoder(const G711Decoder &) = delete;
    G711Decoder &operator=(const G711Decoder &) = delete;

    // Initialize the decoder.
    //   outputFormat : sampleRate must be 8000; channels 1; bitsPerSample == 16.
    //   variant      : MuLaw or ALaw.
    bool begin(const PCMFormat &outputFormat, G711Variant variant);
    void end();

    // Runtime variant swap.
    void setVariant(G711Variant v) { variant_ = v; }
    G711Variant variant() const { return variant_; }

    // Decode `packetBytes` G.711 bytes into `pcm`. PCM capacity must be
    // >= packetBytes (one sample per byte). Returns the number of PCM
    // samples produced.
    size_t decode(const uint8_t *packet,
                  size_t packetBytes,
                  int16_t *pcm,
                  size_t maxFrames);

    // PCMSource interface ------------------------------------------------
    // Returns frames previously queued by decode() when the caller routes
    // output through PCMFlow::setInputSource(). The packet stream has no
    // defined EOF, so isEof() is always false.
    const PCMFormat &format() const override { return format_; }
    bool isReady() const override { return ready_; }
    bool isEof() const override { return false; }
    size_t readFrames(void *out, size_t frameCount) override;

    Error lastError() const { return error_; }

private:
    PCMFormat format_{};
    G711Variant variant_ = G711Variant::MuLaw;
    bool ready_ = false;
    Error error_ = Error::NotReady;
};

#endif // PCMFLOWG711_G711DECODER_H
