"""G711Decoder unit tests.

The on-device sketch feeds all 256 possible byte values to G711Decoder
for both variants and compares against the reference table built by
tests/tools/gen_test_audio.py from the ITU-T G.711 specification.
"""

import re


def test_g711_decoder(dut):
    dut.expect("TEST start g711_decoder", timeout=10)
    match = dut.expect(re.compile(rb"TEST done (\d+)/(\d+)"), timeout=30)
    passed, total = int(match.group(1)), int(match.group(2))
    assert passed == total, f"{total - passed} of {total} assertions failed"
    assert total > 0, "no assertions ran"
