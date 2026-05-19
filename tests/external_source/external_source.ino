// Integration test: G711Decoder as a PCMSource for PCMFlow.
//
// STATUS: placeholder. Will plug a G711Decoder into PCMFlow via
// setInputSource(), feed it a known byte stream, and read PCM back
// through PCMFlow's pump() / readFrames() path.

#include <PCMFlow.h>
#include <PCMFlowG711.h>

void setup()
{
    Serial.begin(115200);
    delay(2000);
    Serial.println("TEST start external_source");
    Serial.println("TEST done 0/0");
}

void loop()
{
    delay(1);
}
