#!/usr/bin/env python3
"""Generate G.711 encode/decode reference tables for the test fixtures.

The G.711 algorithm is defined by ITU-T Recommendation G.711. The
encode/decode mappings are deterministic, so this script produces the
reference tables straight from the specification, without depending on
any third-party codec (ffmpeg, sox, etc.). The resulting tables are
embedded as C arrays under each test's `input/` directory and asserted
bit-exactly by the on-device sketches.

Usage:
    python gen_test_audio.py                # writes all fixtures
    python gen_test_audio.py --print-table  # prints tables to stdout

STATUS: skeleton. The mu-law / A-law formulas below come straight from
the ITU-T G.711 specification; this is an independent implementation of
the published mathematical mapping, not derived from any third-party
source code. They are intentionally written verbosely so the test
harness can be reviewed against the spec line by line.
"""

from __future__ import annotations

import argparse
import pathlib

# -------------------------------------------------------------------- mu-law

# ITU-T G.711 Annex 1: mu = 255.
# Encode: 14-bit signed magnitude PCM (sign-magnitude after biasing) ->
# 8-bit code. Decode: 8-bit code -> 14-bit signed PCM, sign-extended to
# 16-bit by left-shifting by 2 inside the segment formula.

_MULAW_BIAS = 0x84  # 132
_MULAW_CLIP = 32635


def mulaw_encode_sample(pcm16: int) -> int:
    """Encode one 16-bit signed PCM sample to a mu-law byte (0..255)."""
    sign = 0
    if pcm16 < 0:
        pcm16 = -pcm16
        sign = 0x80
    if pcm16 > _MULAW_CLIP:
        pcm16 = _MULAW_CLIP
    pcm16 += _MULAW_BIAS
    # Find the segment: highest bit position above bit 7.
    seg = 7
    mask = 0x4000
    while (pcm16 & mask) == 0 and seg > 0:
        mask >>= 1
        seg -= 1
    mantissa = (pcm16 >> (seg + 3)) & 0x0F
    code = ~(sign | (seg << 4) | mantissa) & 0xFF
    return code


def mulaw_decode_sample(code: int) -> int:
    """Decode one mu-law byte to a 16-bit signed PCM sample."""
    code = ~code & 0xFF
    sign = code & 0x80
    seg = (code >> 4) & 0x07
    mantissa = code & 0x0F
    magnitude = ((mantissa << 3) + _MULAW_BIAS) << seg
    magnitude -= _MULAW_BIAS
    return -magnitude if sign else magnitude


# -------------------------------------------------------------------- A-law

# ITU-T G.711 Annex 2: A = 87.6. Encode operates on 13-bit signed PCM
# (16-bit right-shifted by 3 inside the segment formula).

_ALAW_INVERT = 0x55


def alaw_encode_sample(pcm16: int) -> int:
    sign = 0
    if pcm16 < 0:
        pcm16 = -pcm16 - 1
        sign = 0x80
    if pcm16 > 32767:
        pcm16 = 32767
    if pcm16 < 256:
        seg = 0
        mantissa = (pcm16 >> 4) & 0x0F
    else:
        seg = 1
        v = pcm16 >> 4
        while v >= 32:
            v >>= 1
            seg += 1
        mantissa = (pcm16 >> (seg + 3)) & 0x0F
    code = (sign | (seg << 4) | mantissa) ^ _ALAW_INVERT
    return code & 0xFF


def alaw_decode_sample(code: int) -> int:
    code ^= _ALAW_INVERT
    sign = code & 0x80
    seg = (code >> 4) & 0x07
    mantissa = code & 0x0F
    if seg == 0:
        magnitude = (mantissa << 4) + 8
    else:
        magnitude = ((mantissa << 4) + 0x108) << (seg - 1)
    return -magnitude if sign else magnitude


# -------------------------------------------------------------------- driver


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--print-table", action="store_true",
                   help="Print the 256-entry decode tables for both variants and exit.")
    return p.parse_args()


def main() -> None:
    args = parse_args()
    if args.print_table:
        for name, fn in [("mu", mulaw_decode_sample), ("A", alaw_decode_sample)]:
            print(f"# {name}-law decode table (256 entries, 16-bit signed):")
            for c in range(256):
                print(f"  {c:3d} -> {fn(c):+6d}")
        return

    # TODO: write reference tables / encoded PCM ramps under the
    # appropriate tests/*/input/ directories. Wired up alongside the
    # codec implementation.
    here = pathlib.Path(__file__).resolve().parent
    print(f"[gen_test_audio.py] placeholder — fixtures will be written under {here.parent}/.../input/")


if __name__ == "__main__":
    main()
