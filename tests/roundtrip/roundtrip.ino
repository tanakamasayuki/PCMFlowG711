// End-to-end encode -> decode roundtrip tests for PCMFlowG711.
//
// G.711 is lossy, so input PCM does NOT survive intact in general; what
// IS deterministic is the encode-then-decode mapping. The reference
// fixture under input/reference.h carries:
//   - a swept PCM input array,
//   - the expected post-roundtrip PCM for mu-law,
//   - the expected post-roundtrip PCM for A-law.
// Both were computed independently in Python from the ITU-T spec.
//
// In addition to the bit-exact equality check, we verify two cheap
// invariants the spec guarantees:
//   - sign preservation (encode/decode never flips sign except at the
//     ambiguous zero boundary),
//   - per-sample quantization error stays within the segment step.

#include <PCMFlowG711.h>

#include "input/reference.h"

static int g_pass = 0;
static int g_total = 0;

#define EXPECT_TRUE(name, cond) do { \
    ++g_total; \
    if (cond) { ++g_pass; Serial.print("PASS "); Serial.println(name); } \
    else { Serial.print("FAIL "); Serial.print(name); Serial.println(" cond"); } \
} while (0)

#define EXPECT_EQ(name, expected, actual) do { \
    ++g_total; \
    const long _e = (long)(expected); \
    const long _a = (long)(actual); \
    if (_e == _a) { ++g_pass; Serial.print("PASS "); Serial.println(name); } \
    else { \
        Serial.print("FAIL "); Serial.print(name); \
        Serial.print(" expected="); Serial.print(_e); \
        Serial.print(" actual=");   Serial.println(_a); \
    } \
} while (0)

// Worst-case quantization step at the largest mu-law segment is
// (2^seg) * 16; for seg=7 that's 2048. A-law top segment step is 256.
// Use the looser bound for the generic tolerance check.
static const long kRoundtripTolerance = 2048;

static void test_roundtrip(G711Variant variant, const int16_t *expected, const char *tag)
{
    G711Encoder enc;
    G711Decoder dec;
    EXPECT_TRUE("enc.begin", enc.begin({8000, 1, 16}, variant));
    EXPECT_TRUE("dec.begin", dec.begin({8000, 1, 16}, variant));

    // Encode in place into a scratch byte buffer, then decode back.
    static uint8_t bytes[kG711RoundtripCount];
    static int16_t decoded[kG711RoundtripCount];

    const size_t enc_got = enc.encode(kG711RoundtripPcm,
                                      kG711RoundtripCount,
                                      bytes,
                                      kG711RoundtripCount);
    EXPECT_EQ("enc.count", (long)kG711RoundtripCount, (long)enc_got);

    const size_t dec_got = dec.decode(bytes,
                                      kG711RoundtripCount,
                                      decoded,
                                      kG711RoundtripCount);
    EXPECT_EQ("dec.count", (long)kG711RoundtripCount, (long)dec_got);

    int mismatches = 0;
    int sign_flips = 0;
    long worst_err = 0;
    int first_bad = -1;
    for (size_t i = 0; i < kG711RoundtripCount; ++i) {
        if (decoded[i] != expected[i]) {
            ++mismatches;
            if (first_bad < 0) first_bad = (int)i;
        }
        // Sign preservation: ignore the zero-bucket where the spec
        // collapses [-1, 0] into a single codeword that decodes to 0.
        const int16_t in = kG711RoundtripPcm[i];
        if (in > 0 && decoded[i] < 0) ++sign_flips;
        if (in < -1 && decoded[i] > 0) ++sign_flips;
        const long err = (long)decoded[i] - (long)in;
        const long abserr = err >= 0 ? err : -err;
        if (abserr > worst_err) worst_err = abserr;
    }

    ++g_total;
    if (mismatches == 0) {
        ++g_pass;
        Serial.print("PASS "); Serial.print(tag); Serial.println("/bit-exact-vs-reference");
    } else {
        Serial.print("FAIL "); Serial.print(tag); Serial.print("/bit-exact-vs-reference");
        Serial.print(" mismatches="); Serial.print(mismatches);
        Serial.print(" first_idx="); Serial.println(first_bad);
    }

    char name[40];
    snprintf(name, sizeof(name), "%s/sign-preserved", tag);
    EXPECT_EQ(name, 0, sign_flips);

    snprintf(name, sizeof(name), "%s/quant-err-within-bound", tag);
    EXPECT_TRUE(name, worst_err <= kRoundtripTolerance);
}

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("TEST start roundtrip");

    test_roundtrip(G711Variant::MuLaw, kG711RoundtripMuLaw, "mu");
    test_roundtrip(G711Variant::ALaw,  kG711RoundtripALaw,  "a");

    Serial.print("TEST done ");
    Serial.print(g_pass);
    Serial.print("/");
    Serial.println(g_total);
}

void loop()
{
    delay(1);
}
