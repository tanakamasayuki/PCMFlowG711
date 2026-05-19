#include "G711Encoder.h"

// PCMFlowG711 :: G711Encoder implementation.
//
// Clean-room implementation of ITU-T G.711 mu-law (Annex 1) and A-law
// (Annex 2) companding. Each PCM sample is mapped to one 8-bit code by
// a stateless segment-based formula; no precomputed encode table is
// needed.
//
// The encoder holds no per-sample state, so begin() / encode() are
// safe to call from any task context. writeFrames() (the PCMSink path)
// processes input in fixed-size stack chunks and hands each chunk to
// the registered PacketSink callback.

namespace
{
    constexpr int kMuLawBias = 0x84;  // 132, per G.711 Annex 1
    constexpr int kMuLawClip = 32635; // 32767 - 132
    constexpr uint8_t kALawInvert = 0x55;

    // Encode one 16-bit signed PCM sample to a mu-law byte.
    inline uint8_t encodeMuLaw(int16_t pcm)
    {
        uint8_t sign = 0;
        int32_t mag = pcm;
        if (mag < 0)
        {
            mag = -mag;
            sign = 0x80;
        }
        if (mag > kMuLawClip)
        {
            mag = kMuLawClip;
        }
        mag += kMuLawBias;

        // Locate the segment: highest set bit position of `mag` between
        // bit 7 and bit 14 (mag is at most 0x7FFF + bias).
        int seg = 7;
        int mask = 0x4000;
        while ((mag & mask) == 0 && seg > 0)
        {
            mask >>= 1;
            --seg;
        }
        const int mantissa = (mag >> (seg + 3)) & 0x0F;
        return static_cast<uint8_t>(~(sign | (seg << 4) | mantissa) & 0xFF);
    }

    // Encode one 16-bit signed PCM sample to an A-law byte.
    inline uint8_t encodeALaw(int16_t pcm)
    {
        // A-law convention: in the raw (pre-XOR-with-0x55) byte, bit 7 == 1
        // denotes positive. Negative magnitudes use one's-complement of the
        // (negated minus 1) value.
        uint8_t sign;
        int32_t mag = pcm;
        if (mag >= 0)
        {
            sign = 0x80;
        }
        else
        {
            sign = 0x00;
            mag = -mag - 1;
        }
        if (mag > 32767)
        {
            mag = 32767;
        }

        int seg;
        int mantissa;
        if (mag < 256)
        {
            seg = 0;
            mantissa = (mag >> 4) & 0x0F;
        }
        else
        {
            seg = 1;
            int v = mag >> 4;
            while (v >= 32)
            {
                v >>= 1;
                ++seg;
            }
            mantissa = (mag >> (seg + 3)) & 0x0F;
        }
        return static_cast<uint8_t>((sign | (seg << 4) | mantissa) ^ kALawInvert);
    }
} // namespace

bool G711Encoder::begin(const PCMFormat &inputFormat, G711Variant variant)
{
    if (inputFormat.sampleRate != 8000 || inputFormat.channels != 1 ||
        inputFormat.bitsPerSample != 16)
    {
        error_ = Error::InvalidFormat;
        ready_ = false;
        return false;
    }
    format_ = inputFormat;
    variant_ = variant;
    ready_ = true;
    error_ = Error::None;
    return true;
}

void G711Encoder::end()
{
    ready_ = false;
    error_ = Error::NotReady;
    packetSinkCb_ = nullptr;
    packetSinkUser_ = nullptr;
}

size_t G711Encoder::encode(const int16_t *pcm,
                           size_t frameCount,
                           uint8_t *out,
                           size_t outCapacity)
{
    if (!ready_ || pcm == nullptr || out == nullptr)
    {
        error_ = ready_ ? Error::InvalidFormat : Error::NotReady;
        return 0;
    }
    if (outCapacity < frameCount)
    {
        error_ = Error::BufferTooSmall;
        return 0;
    }

    if (variant_ == G711Variant::MuLaw)
    {
        for (size_t i = 0; i < frameCount; ++i)
        {
            out[i] = encodeMuLaw(pcm[i]);
        }
    }
    else
    {
        for (size_t i = 0; i < frameCount; ++i)
        {
            out[i] = encodeALaw(pcm[i]);
        }
    }
    error_ = Error::None;
    return frameCount;
}

size_t G711Encoder::writeFrames(const void *in, size_t frameCount)
{
    if (!ready_ || in == nullptr || packetSinkCb_ == nullptr)
    {
        return 0;
    }
    const int16_t *pcm = static_cast<const int16_t *>(in);

    // Process in stack-sized chunks so writeFrames() with large inputs
    // doesn't require an unbounded scratch buffer.
    constexpr size_t kChunkBytes = 256;
    uint8_t chunk[kChunkBytes];
    size_t consumed = 0;
    while (consumed < frameCount)
    {
        const size_t take = (frameCount - consumed) < kChunkBytes
                                ? (frameCount - consumed)
                                : kChunkBytes;
        const size_t got = encode(pcm + consumed, take, chunk, kChunkBytes);
        if (got == 0)
        {
            break;
        }
        packetSinkCb_(packetSinkUser_, chunk, got);
        consumed += got;
    }
    return consumed;
}

void G711Encoder::setPacketSink(PacketSink cb, void *userData)
{
    packetSinkCb_ = cb;
    packetSinkUser_ = userData;
}
