"""G711Decoder + PCMFlow::setInputSource() integration test.

Placeholder — implementation lands together with the codec.
"""


def test_external_source(dut):
    dut.expect("TEST start external_source", timeout=10)
    dut.expect("TEST done", timeout=30)
