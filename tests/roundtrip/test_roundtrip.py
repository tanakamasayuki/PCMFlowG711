"""Encode → decode round-trip test.

Placeholder — implementation lands together with the codec.
"""


def test_roundtrip(dut):
    dut.expect("TEST start roundtrip", timeout=10)
    dut.expect("TEST done", timeout=30)
