// Integration test: G711Decoder plugged into PCMFlow as a PCMSource.
//
// The decoder's PCMSource path (decode(packet, n, nullptr, 0) +
// readFrames()) is exercised through PCMFlow's pipeline:
//   1. test enqueues bytes via decoder.decode(..., nullptr, 0)
//   2. PCMFlow::pump() pulls PCM out of the decoder via readFrames()
//   3. test reads PCM from PCMFlow via readFrames()
// and compares the result against the independently-computed reference
// PCM in input/reference.h.
//
// Beyond the bit-exact passthrough check, the test also covers
// PCMFlow's downstream transforms (mono->stereo up-mix, gain, 8 kHz ->
// 16 kHz resample) to confirm the G.711 source plays nicely with them.

#include <PCMFlow.h>
#include <PCMFlowG711.h>

#include "input/reference.h"

static int g_pass = 0;
static int g_total = 0;

#define EXPECT_TRUE(name, cond)      \
    do                               \
    {                                \
        ++g_total;                   \
        if (cond)                    \
        {                            \
            ++g_pass;                \
            Serial.print("PASS ");   \
            Serial.println(name);    \
        }                            \
        else                         \
        {                            \
            Serial.print("FAIL ");   \
            Serial.print(name);      \
            Serial.println(" cond"); \
        }                            \
    } while (0)

#define EXPECT_EQ(name, expected, actual) \
    do                                    \
    {                                     \
        ++g_total;                        \
        const long _e = (long)(expected); \
        const long _a = (long)(actual);   \
        if (_e == _a)                     \
        {                                 \
            ++g_pass;                     \
            Serial.print("PASS ");        \
            Serial.println(name);         \
        }                                 \
        else                              \
        {                                 \
            Serial.print("FAIL ");        \
            Serial.print(name);           \
            Serial.print(" expected=");   \
            Serial.print(_e);             \
            Serial.print(" actual=");     \
            Serial.println(_a);           \
        }                                 \
    } while (0)

#define EXPECT_NEAR(name, expected, actual, tol)           \
    do                                                     \
    {                                                      \
        ++g_total;                                         \
        const long _e = (long)(expected);                  \
        const long _a = (long)(actual);                    \
        const long _d = (_e > _a) ? (_e - _a) : (_a - _e); \
        if (_d <= (long)(tol))                             \
        {                                                  \
            ++g_pass;                                      \
            Serial.print("PASS ");                         \
            Serial.println(name);                          \
        }                                                  \
        else                                               \
        {                                                  \
            Serial.print("FAIL ");                         \
            Serial.print(name);                            \
            Serial.print(" expected=");                    \
            Serial.print(_e);                              \
            Serial.print(" actual=");                      \
            Serial.print(_a);                              \
            Serial.print(" diff=");                        \
            Serial.println(_d);                            \
        }                                                  \
    } while (0)

// G.711 has no defined EOF, so we drive read loops by expected sample
// count rather than isEof(). pump_and_drain feeds bytes into the
// decoder in queue-sized chunks (the decoder FIFO is 320 bytes, the
// test pattern is 160 bytes; one chunk is enough but the helper
// generalizes to longer patterns too).
static size_t pump_and_drain(PCMFlow &audio,
                             G711Decoder &dec,
                             const uint8_t *bytes,
                             size_t byteCount,
                             int16_t *out,
                             size_t outCap)
{
    size_t enq = 0;
    size_t produced = 0;
    int idle = 0;
    while (produced < outCap && idle < 32)
    {
        if (enq < byteCount)
        {
            const size_t took = dec.decode(bytes + enq, byteCount - enq,
                                           nullptr, 0);
            enq += took;
        }
        audio.pump();
        const size_t got = audio.readFrames(out + produced, outCap - produced);
        if (got == 0)
        {
            ++idle;
        }
        else
        {
            idle = 0;
            produced += got;
        }
    }
    return produced;
}

static void test_passthrough(G711Variant variant,
                             const int16_t *expected,
                             const char *tag)
{
    G711Decoder dec;
    EXPECT_TRUE("dec.begin", dec.begin({8000, 1, 16}, variant));
    EXPECT_TRUE("dec.ready", dec.isReady());
    EXPECT_TRUE("dec.eof-always-false", !dec.isEof());

    PCMFlow audio;
    audio.setInputSource(dec);
    audio.setOutputFormat({8000, 1, 16});

    static int16_t out[kG711ExtSourceByteCount];
    const size_t produced = pump_and_drain(audio, dec,
                                           kG711ExtSourceBytes,
                                           kG711ExtSourceByteCount,
                                           out,
                                           kG711ExtSourceByteCount);

    char name[48];
    snprintf(name, sizeof(name), "%s/pass/count", tag);
    EXPECT_EQ(name, (long)kG711ExtSourceByteCount, (long)produced);

    int mismatches = 0;
    int first_bad = -1;
    for (size_t i = 0; i < produced; ++i)
    {
        if (out[i] != expected[i])
        {
            ++mismatches;
            if (first_bad < 0)
                first_bad = (int)i;
        }
    }
    ++g_total;
    if (mismatches == 0 && produced == kG711ExtSourceByteCount)
    {
        ++g_pass;
        Serial.print("PASS ");
        Serial.print(tag);
        Serial.println("/pass/bit-exact");
    }
    else
    {
        Serial.print("FAIL ");
        Serial.print(tag);
        Serial.print("/pass/bit-exact");
        Serial.print(" mismatches=");
        Serial.print(mismatches);
        Serial.print(" first_idx=");
        Serial.println(first_bad);
    }
}

static void test_mono_to_stereo_with_gain()
{
    G711Decoder dec;
    dec.begin({8000, 1, 16}, G711Variant::MuLaw);

    PCMFlow audio;
    audio.setInputSource(dec);
    audio.setOutputFormat({8000, 2, 16});
    audio.setGain(0.5f);

    static int16_t out[kG711ExtSourceByteCount * 2];
    // 1 mono frame in -> 1 stereo frame out -> 2 int16 written per
    // input sample. PCMFlow::readFrames takes a frame count, so the
    // helper's outCap is in frames, but the storage we provide above
    // is sized for 2*samples.
    const size_t produced = pump_and_drain(audio, dec,
                                           kG711ExtSourceBytes,
                                           kG711ExtSourceByteCount,
                                           out,
                                           kG711ExtSourceByteCount);
    EXPECT_EQ("mono2stereo/count", (long)kG711ExtSourceByteCount, (long)produced);

    // For every output frame: L == R (mono up-mixed by duplication) and
    // amplitude is gain-scaled. Verify on a representative subset.
    int channel_diffs = 0;
    long worst_gain_err = 0;
    for (size_t i = 0; i < produced; ++i)
    {
        const int16_t l = out[2 * i + 0];
        const int16_t r = out[2 * i + 1];
        if (l != r)
            ++channel_diffs;
        const long expected_scaled = (long)kG711ExtSourceExpectedMuLaw[i] / 2;
        const long err = (long)l - expected_scaled;
        const long abserr = err >= 0 ? err : -err;
        if (abserr > worst_gain_err)
            worst_gain_err = abserr;
    }
    EXPECT_EQ("mono2stereo/L==R", 0, channel_diffs);
    // 0.5 gain via integer divide loses at most 1 LSB.
    EXPECT_TRUE("mono2stereo/gain-within-1lsb", worst_gain_err <= 1);
}

static void test_resample_8k_to_16k()
{
    G711Decoder dec;
    dec.begin({8000, 1, 16}, G711Variant::MuLaw);

    PCMFlow audio;
    audio.setInputSource(dec);
    audio.setOutputFormat({16000, 1, 16}); // 2x upsample

    static int16_t out[kG711ExtSourceByteCount * 2 + 8];
    const size_t produced = pump_and_drain(audio, dec,
                                           kG711ExtSourceBytes,
                                           kG711ExtSourceByteCount,
                                           out,
                                           kG711ExtSourceByteCount * 2);
    // 2x upsample of 160 frames -> ~320 frames (allow a few frame
    // boundary effects from linear interp).
    EXPECT_NEAR("resample/total", (long)kG711ExtSourceByteCount * 2,
                (long)produced, 4);
}

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("TEST start external_source");

    test_passthrough(G711Variant::MuLaw, kG711ExtSourceExpectedMuLaw, "mu");
    test_passthrough(G711Variant::ALaw, kG711ExtSourceExpectedALaw, "a");
    test_mono_to_stereo_with_gain();
    test_resample_8k_to_16k();

    Serial.print("TEST done ");
    Serial.print(g_pass);
    Serial.print("/");
    Serial.println(g_total);
}

void loop()
{
    delay(1);
}
