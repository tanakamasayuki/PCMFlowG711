// Smoke test sketch — verifies the PCMFlowG711 library compiles against
// the chosen profile and that the test harness wiring works.
// Once the codec implementation lands, this stays as a build-only check.

#include <PCMFlowG711.h>

void setup()
{
    Serial.begin(115200);
    delay(2000);
    Serial.print("PCMFlowG711 ");
    Serial.println(PCMFLOWG711_VERSION_STR);
    Serial.println("SMOKE ready");
}

void loop()
{
    delay(1);
}
