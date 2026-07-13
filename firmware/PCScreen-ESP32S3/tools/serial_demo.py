#!/usr/bin/env python3
"""Send demo telemetry to PCScreen over USB CDC.

Usage:
  python -m pip install pyserial
  python serial_demo.py COM5
  python serial_demo.py /dev/cu.usbmodemXXXX
"""
import json
import math
import sys
import time

import serial


def send(port: serial.Serial, message: dict) -> None:
    port.write((json.dumps(message, separators=(",", ":")) + "\n").encode())


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: python serial_demo.py <serial-port>")
        return 2
    with serial.Serial(sys.argv[1], 115200, timeout=0.1) as port:
        time.sleep(1.5)
        send(port, {"type": "hello"})
        started = time.monotonic()
        while True:
            elapsed = time.monotonic() - started
            send(port, {
                "type": "telemetry",
                "cpuTemp": 52 + 5 * math.sin(elapsed / 4),
                "cpuLoad": 44 + 25 * math.sin(elapsed / 2),
                "cpuClockMHz": 4750 + 250 * math.sin(elapsed / 3),
                "cpuPowerWatts": 72 + 18 * math.sin(elapsed / 2),
                "cpuFanRpm": 920 + 180 * math.sin(elapsed / 4),
                "gpuTemp": 61 + 4 * math.sin(elapsed / 5),
                "gpuLoad": 72 + 20 * math.sin(elapsed / 3),
                "gpuClockMHz": 2520 + 120 * math.sin(elapsed / 3),
                "gpuPowerWatts": 175 + 35 * math.sin(elapsed / 3),
                "gpuFanRpm": 1380 + 220 * math.sin(elapsed / 4),
                "gpuMemoryLoad": 46 + 8 * math.sin(elapsed / 5),
                "ramLoad": 58,
                "memoryUsedGb": 18.6,
                "memoryTotalGb": 32.0,
                "fps": 144 + 8 * math.sin(elapsed),
            })
            response = port.readline().decode(errors="replace").strip()
            if response:
                print(response)
            time.sleep(0.5)


if __name__ == "__main__":
    raise SystemExit(main())
