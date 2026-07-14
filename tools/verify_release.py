#!/usr/bin/env python3
"""Offline integrity checks for the PCScreen public release."""

from __future__ import annotations

import csv
import hashlib
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "release-assets"
PARTITIONS = ROOT / "firmware" / "partitions_4mb_assets.csv"
OTA = ASSETS / "PCScreen-S3-4MB-v1.7.0.pcota"
FIRMWARE = ASSETS / "firmware.bin"
MAGIC = b"PCSOTA1\0"


def verify_partitions() -> None:
    rows: list[tuple[str, int, int]] = []
    lines = (line for line in PARTITIONS.read_text().splitlines()
             if line and not line.startswith("#"))
    for row in csv.reader(lines):
        name, _, _, offset, size, *_ = (item.strip() for item in row)
        rows.append((name, int(offset, 0), int(size, 0)))
    for left, right in zip(rows, rows[1:]):
        assert left[1] + left[2] <= right[1], f"overlap: {left[0]}"
    assert rows[-1][1] + rows[-1][2] == 0x400000
    sizes = dict((name, size) for name, _, size in rows)
    assert sizes["app0"] == sizes["app1"] == 0x160000
    assert sizes["spiffs"] == 0x130000


def verify_ota() -> None:
    data = OTA.read_bytes()
    magic, version, payload_size = struct.unpack("<8sII", data[:16])
    assert magic == MAGIC and version == 1
    assert payload_size == len(data) - 16
    assert data[16:] == FIRMWARE.read_bytes()
    assert data[16] == 0xE9


def verify_checksums() -> None:
    for line in (ASSETS / "SHA256SUMS.txt").read_text().splitlines():
        expected, name = line.split(maxsplit=1)
        path = ASSETS / name.strip()
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
        assert actual == expected, f"SHA-256 mismatch: {path.name}"


if __name__ == "__main__":
    verify_partitions()
    verify_ota()
    verify_checksums()
    print("OK: partitions, PCOTA payload and SHA-256 checksums are valid.")
