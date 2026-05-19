#ifndef PCMFLOWG711_G711ENCODER_H
#define PCMFLOWG711_G711ENCODER_H

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

#include "PCMFormat.h"
#include "PCMSink.h"
#include "G711Variant.h"

// PCMFlowG711 :: G711Encoder
//
// Encodes 16-bit signed PCM samples (8 kHz mono) to 8-bit G.711 bytes
// (mu-law or A-law). One PCM sample maps to exactly one G.711 byte;
// there is no internal framing.
//
// The encoder is stateless beyond the configured variant: a single
// begin() with G711Variant fixes the curve, and subsequent encode()
// calls are independent of one another. Safe to call from any task
// context once begin() has returned.
//
// STATUS: header skeleton only. Implementation is forthcoming.
//         Signatures below are SPEC §4.2 sketches.

class G711Encoder : public PCMSink
{
public:
    enum class Error : uint8_t
    {
        None,
        NotReady,
        InvalidFormat,  // sample rate != 8000, channels != 1, or bits != 16
        BufferTooSmall, // output capacity < frameCount
    };

    G711Encoder() = default;
    ~G711Encoder() = default;

    G711Encoder(const G711Encoder &) = delete;
    G711Encoder &operator=(const G711Encoder &) = delete;

    // Initialize the encoder.
    //   inputFormat : sampleRate must be 8000; channels 1; bitsPerSample == 16.
    //   variant     : MuLaw or ALaw.
    // Returns false on error; query lastError() for the cause.
    bool begin(const PCMFormat &inputFormat, G711Variant variant);
    void end();

    // Runtime variant swap. The encoder remains stateless across the
    // swap; the next encode()/writeFrames() call uses the new curve.
    void setVariant(G711Variant v) { variant_ = v; }
    G711Variant variant() const { return variant_; }

    // Encode `frameCount` PCM samples into `out`. Output capacity must
    // be >= frameCount (one byte per sample). Returns the number of
    // bytes written, or 0 on error.
    size_t encode(const int16_t *pcm,
                  size_t frameCount,
                  uint8_t *out,
                  size_t outCapacity);

    // PCMSink interface --------------------------------------------------
    // Pushes PCM into the encoder. If a packet sink callback is registered
    // via setPacketSink(), each writeFrames() emits one encoded chunk of
    // bytes (same length as the input sample count) to it. Without a sink
    // the PCMSink path is a no-op; use encode() for direct control.
    const PCMFormat &format() const override { return format_; }
    bool isReady() const override { return ready_; }
    size_t writeFrames(const void *in, size_t frameCount) override;

    Error lastError() const { return error_; }

    // Packet sink callback signature for the PCMSink path.
    using PacketSink = void (*)(void *userData,
                                const uint8_t *packet,
                                size_t packetBytes);
    void setPacketSink(PacketSink cb, void *userData);

private:
    PCMFormat format_{};
    G711Variant variant_ = G711Variant::MuLaw;
    bool ready_ = false;
    Error error_ = Error::NotReady;

    PacketSink packetSinkCb_ = nullptr;
    void *packetSinkUser_ = nullptr;
};

#endif // PCMFLOWG711_G711ENCODER_H
