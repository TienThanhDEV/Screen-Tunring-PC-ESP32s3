#!/usr/bin/env python3
"""Pack a PlatformIO application firmware.bin into one safe PCScreen OTA file."""

from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path

MAGIC = b"PCSOTA1\0"
FORMAT_VERSION = 1
HEADER = struct.Struct("<8sII")


def main() -> int:
    parser = argparse.ArgumentParser(description="Create a PCScreen .pcota bundle")
    parser.add_argument("firmware", type=Path, help="PlatformIO firmware.bin")
    parser.add_argument("output", type=Path, help="output .pcota file")
    args = parser.parse_args()

    payload = args.firmware.read_bytes()
    if not payload or payload[0] != 0xE9:
        parser.error("input is not an ESP32 application image (missing 0xE9 magic)")
    if len(payload) > 0x140000:
        parser.error("firmware exceeds the 4 MB layout OTA slot (0x140000 bytes)")
    if args.output.suffix.lower() != ".pcota":
        parser.error("output filename must end with .pcota")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(HEADER.pack(MAGIC, FORMAT_VERSION, len(payload)) + payload)
    digest = hashlib.sha256(args.output.read_bytes()).hexdigest()
    print(f"Created: {args.output}")
    print(f"Firmware payload: {len(payload)} bytes")
    print(f"SHA-256: {digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
