"""Encode -> decode roundtrip test for both G.711 variants.

The on-device sketch encodes a swept PCM input, decodes it back, and
compares against the independently-computed reference roundtrip table.
"""

import re


def test_roundtrip(dut):
    dut.expect("TEST start roundtrip", timeout=10)
    match = dut.expect(re.compile(rb"TEST done (\d+)/(\d+)"), timeout=30)
    passed, total = int(match.group(1)), int(match.group(2))
    assert passed == total, f"{total - passed} of {total} assertions failed"
    assert total > 0, "no assertions ran"
