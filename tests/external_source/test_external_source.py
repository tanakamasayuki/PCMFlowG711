"""G711Decoder + PCMFlow::setInputSource() integration test.

The on-device sketch wires a G711Decoder into PCMFlow as a PCMSource,
feeds it a known G.711 byte stream via the queued decode() path, and
reads PCM out through PCMFlow's pump()/readFrames() pipeline. Output
is compared bit-exactly against the independently-computed reference.
"""

import re


def test_external_source(dut):
    dut.expect("TEST start external_source", timeout=10)
    match = dut.expect(re.compile(rb"TEST done (\d+)/(\d+)"), timeout=30)
    passed, total = int(match.group(1)), int(match.group(2))
    assert passed == total, f"{total - passed} of {total} assertions failed"
    assert total > 0, "no assertions ran"
