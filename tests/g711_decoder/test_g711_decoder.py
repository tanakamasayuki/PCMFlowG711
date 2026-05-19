"""G711Decoder unit tests.

Placeholder — implementation lands together with G711Decoder itself.
"""


def test_g711_decoder(dut):
    dut.expect("TEST start g711_decoder", timeout=10)
    dut.expect("TEST done", timeout=30)
