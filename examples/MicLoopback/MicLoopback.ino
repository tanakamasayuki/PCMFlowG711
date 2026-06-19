// PCMFlowG711 example: MicLoopback
//
// Single-board demo that proves the codec end-to-end with just one M5
// Core2 (no second board, no ESP-NOW). Useful as a sanity check before
// trying the EspNowTransceiver example.
//
// Pipeline:
//   M5.Mic (8 kHz mono) -> G711Encoder -> G711Decoder -> PCMFlow -> M5.Speaker
//
// Hold button A : speak into the mic; the speaker plays back what you
//                 said, after a full mu-law encode + decode roundtrip.
// Release       : stop.
//
// Switch the codec to A-law by changing kVariant below.

#include <M5Unified.h>
#include <PCMFlow.h>
#include <PCMFlowDeviceM5.h>
#include <PCMFlowG711.h>

static constexpr G711Variant kVariant = G711Variant::MuLaw;
static constexpr uint32_t kRate = 8000;
static constexpr uint32_t kSpeakerRate = 16000;
static constexpr size_t kFrameSamples = 160; // 20 ms @ 8 kHz
static constexpr size_t kUpsampleRatio = kSpeakerRate / kRate;
static constexpr size_t kMaxPlayFrames = (kSpeakerRate * 80u) / 1000u;
using Player = M5SpeakerBufferedPlayer<kMaxPlayFrames>;

static G711Encoder g_enc;
static G711Decoder g_dec;
static PCMFlow g_audio;
static Player g_player;

static void capture_encode_decode_one_frame()
{
    static int16_t pcm[kFrameSamples];
    if (!M5.Mic.record(pcm, kFrameSamples, kRate, /*stereo=*/false))
        return;
    while (M5.Mic.isRecording())
        delay(1);

    uint8_t packet[kFrameSamples];
    const size_t n = g_enc.encode(pcm, kFrameSamples, packet, sizeof(packet));
    if (n == 0)
        return;

    // Loop the packet back into the decoder's PCMSource queue; PCMFlow
    // will pull and play it on the next pump().
    g_dec.decode(packet, n, nullptr, 0);
}

static void play_pending_audio()
{
    g_audio.pump();
    while (g_audio.availableFrames() >= kFrameSamples)
    {
        static int16_t buf[kFrameSamples];
        static int16_t speakerPcm[kFrameSamples * kUpsampleRatio];
        const size_t got = g_audio.readFrames(buf, kFrameSamples);
        if (got == 0)
            break;
        size_t speakerFrames = 0;
        for (size_t i = 0; i < got; ++i)
        {
            for (size_t j = 0; j < kUpsampleRatio; ++j)
                speakerPcm[speakerFrames++] = buf[i];
        }
        g_player.writeFrames(speakerPcm, speakerFrames);
    }
}

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Mic.begin();
    M5.Speaker.begin();
    M5.Speaker.setVolume(160);
    Serial.begin(115200);

    M5.Display.setTextSize(2);
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setCursor(4, 4);
    M5.Display.print("PCMFlowG711");
    M5.Display.setCursor(4, 28);
    M5.Display.print("Mic Loopback");
    M5.Display.setCursor(4, 60);
    M5.Display.print("hold A: speak");

    if (!g_enc.begin({kRate, 1, 16}, kVariant) ||
        !g_dec.begin({kRate, 1, 16}, kVariant))
    {
        Serial.println("codec begin failed");
        return;
    }

    g_audio.setOutputFormat({kRate, 1, 16});
    g_audio.setBufferFrames(2048);
    g_audio.setInputSource(g_dec);

    if (!g_player.begin({kSpeakerRate, 1, 16}, Player::stableProfile()))
    {
        Serial.println("player begin failed");
        return;
    }
}

void loop()
{
    M5.update();

    if (M5.BtnA.isPressed())
    {
        capture_encode_decode_one_frame();
    }

    play_pending_audio();
}
