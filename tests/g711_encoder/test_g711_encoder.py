"""G711Encoder unit tests.

Placeholder — implementation lands together with G711Encoder itself.
"""


def test_g711_encoder(dut):
    dut.expect("TEST start g711_encoder", timeout=10)
    dut.expect("TEST done", timeout=30)
