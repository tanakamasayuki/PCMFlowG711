#include "G711Decoder.h"

#include <string.h>

// PCMFlowG711 :: G711Decoder implementation.
//
// Clean-room implementation of ITU-T G.711 mu-law (Annex 1) and A-law
// (Annex 2) decoding. Each codeword maps to one 16-bit signed PCM
// sample by a stateless segment-based formula; no precomputed decode
// table is used at runtime (the function below compiles to a small
// loop the compiler can inline).
//
// The PCMSource path (readFrames(), used when the decoder is wired
// into PCMFlow::setInputSource()) keeps a small FIFO of yet-to-be-
// decoded encoded bytes. decode() invoked with a non-null pcm buffer
// writes to that buffer directly and does NOT touch the FIFO; decode()
// invoked with pcm == nullptr enqueues the bytes for later readFrames().

namespace
{
    constexpr int kMuLawBias = 0x84;
    constexpr uint8_t kALawInvert = 0x55;

    inline int16_t decodeMuLaw(uint8_t code)
    {
        const uint8_t v = static_cast<uint8_t>(~code);
        const int sign = v & 0x80;
        const int seg = (v >> 4) & 0x07;
        const int mantissa = v & 0x0F;
        int magnitude = ((mantissa << 3) + kMuLawBias) << seg;
        magnitude -= kMuLawBias;
        return static_cast<int16_t>(sign ? -magnitude : magnitude);
    }

    inline int16_t decodeALaw(uint8_t code)
    {
        const uint8_t v = static_cast<uint8_t>(code ^ kALawInvert);
        const int sign = v & 0x80; // 1 = positive in the pre-XOR encoding
        const int seg = (v >> 4) & 0x07;
        const int mantissa = v & 0x0F;
        int magnitude;
        if (seg == 0)
        {
            magnitude = (mantissa << 4) + 8;
        }
        else
        {
            magnitude = ((mantissa << 4) + 0x108) << (seg - 1);
        }
        return static_cast<int16_t>(sign ? magnitude : -magnitude);
    }
} // namespace

bool G711Decoder::begin(const PCMFormat &outputFormat, G711Variant variant)
{
    if (outputFormat.sampleRate != 8000 || outputFormat.channels != 1 ||
        outputFormat.bitsPerSample != 16)
    {
        error_ = Error::InvalidFormat;
        ready_ = false;
        return false;
    }
    format_ = outputFormat;
    variant_ = variant;
    head_ = 0;
    tail_ = 0;
    ready_ = true;
    error_ = Error::None;
    return true;
}

void G711Decoder::end()
{
    ready_ = false;
    error_ = Error::NotReady;
    head_ = 0;
    tail_ = 0;
}

size_t G711Decoder::decode(const uint8_t *packet,
                           size_t packetBytes,
                           int16_t *pcm,
                           size_t maxFrames)
{
    if (!ready_ || packet == nullptr)
    {
        error_ = ready_ ? Error::InvalidFormat : Error::NotReady;
        return 0;
    }

    // Direct-decode path: caller provided a PCM buffer.
    if (pcm != nullptr)
    {
        if (maxFrames < packetBytes)
        {
            error_ = Error::BufferTooSmall;
            return 0;
        }
        if (variant_ == G711Variant::MuLaw)
        {
            for (size_t i = 0; i < packetBytes; ++i)
            {
                pcm[i] = decodeMuLaw(packet[i]);
            }
        }
        else
        {
            for (size_t i = 0; i < packetBytes; ++i)
            {
                pcm[i] = decodeALaw(packet[i]);
            }
        }
        error_ = Error::None;
        return packetBytes;
    }

    // Queued path: enqueue bytes for later readFrames(). Bytes that
    // would overflow the FIFO are dropped (oldest-preserving policy is
    // pointless for a stateless codec — callers should size the buffer
    // to their packet cadence).
    size_t enqueued = 0;
    for (size_t i = 0; i < packetBytes; ++i)
    {
        const size_t next = (head_ + 1) % kQueueCapacity;
        if (next == tail_)
        {
            break; // full
        }
        queue_[head_] = packet[i];
        head_ = next;
        ++enqueued;
    }
    error_ = Error::None;
    return enqueued;
}

size_t G711Decoder::readFrames(void *out, size_t frameCount)
{
    if (!ready_ || out == nullptr)
    {
        return 0;
    }
    int16_t *pcm = static_cast<int16_t *>(out);
    size_t produced = 0;
    while (produced < frameCount && tail_ != head_)
    {
        const uint8_t code = queue_[tail_];
        tail_ = (tail_ + 1) % kQueueCapacity;
        pcm[produced++] = (variant_ == G711Variant::MuLaw)
                              ? decodeMuLaw(code)
                              : decodeALaw(code);
    }
    return produced;
}
