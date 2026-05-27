#!/usr/bin/env python3
"""Convert raw s16le mono PCM to a C array for ESP32 flash embedding."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pcm", type=Path, help="Raw PCM file (s16le)")
    parser.add_argument("header", type=Path, help="Output .h path")
    parser.add_argument("symbol", help="C array name, e.g. chrone_alarm_tone_pcm")
    parser.add_argument(
        "--sample-rate",
        type=int,
        default=44100,
        help="Sample rate metadata (default: 44100)",
    )
    parser.add_argument(
        "--cols",
        type=int,
        default=12,
        help="Samples per line in generated array",
    )
    args = parser.parse_args()

    data = args.pcm.read_bytes()
    if len(data) % 2 != 0:
        print("error: PCM length must be even (16-bit samples)", file=sys.stderr)
        return 1

    n_samples = len(data) // 2
    sym_u = args.symbol.upper()
    out: list[str] = [
        "#pragma once",
        "#include <stdint.h>",
        "",
        f"#define {sym_u}_SAMPLE_RATE_HZ  ({args.sample_rate})",
        f"#define {sym_u}_NUM_SAMPLES    ({n_samples}u)",
        f"#define {sym_u}_DURATION_MS    "
        f"(({n_samples}u * 1000u) / {args.sample_rate}u)",
        "",
        f"static const int16_t {args.symbol}[] = {{",
    ]

    row: list[str] = []
    for i in range(0, len(data), 2):
        sample = int.from_bytes(data[i : i + 2], "little", signed=True)
        row.append(str(sample))
        if len(row) >= args.cols:
            out.append("    " + ", ".join(row) + ",")
            row = []
    if row:
        out.append("    " + ", ".join(row) + ",")

    out.append("};")
    out.append("")

    args.header.parent.mkdir(parents=True, exist_ok=True)
    args.header.write_text("\n".join(out), encoding="utf-8")
    print(
        f"wrote {args.header} ({n_samples} samples, "
        f"{n_samples / args.sample_rate:.2f}s, {len(data)} bytes)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
