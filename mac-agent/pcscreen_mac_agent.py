#!/usr/bin/env python3
"""PCScreen telemetry agent v1.4.0 for Intel macOS.

Author: Nguyen Tien Thanh (GitHub: TienThanhDEV)
License: MIT
"""

from __future__ import annotations

import argparse
import ctypes
import fcntl
import glob
import json
import os
import re
import subprocess
import termios
import threading
import time
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple

import serial
from serial.tools import list_ports


def acquire_single_instance():
    """Prevent two agents from interleaving JSON on the same USB CDC port."""
    lock_dir = os.path.expanduser("~/Library/Application Support/PCScreen")
    os.makedirs(lock_dir, exist_ok=True)
    lock_file = open(os.path.join(lock_dir, "mac-agent.lock"), "w")
    try:
        fcntl.flock(lock_file.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        lock_file.close()
        return None
    lock_file.write(str(os.getpid()))
    lock_file.flush()
    return lock_file


def apply_device_rate(response: str, interval: float,
                      paused: bool) -> Tuple[float, bool]:
    """Read the saved sampling rate returned by firmware 1.2.0 or newer."""
    try:
        message = json.loads(response)
        value = message.get("telemetryIntervalMs")
        if isinstance(value, (int, float)):
            interval = max(0.25, min(5.0, float(value) / 1000.0))
        if isinstance(message.get("telemetryPaused"), bool):
            paused = message["telemetryPaused"]
    except (TypeError, ValueError, json.JSONDecodeError):
        pass
    return interval, paused


def sysctl(name: str) -> Optional[str]:
    try:
        return subprocess.check_output(["/usr/sbin/sysctl", "-n", name], text=True,
                                       stderr=subprocess.DEVNULL, timeout=2).strip()
    except (OSError, subprocess.SubprocessError):
        return None


class CpuLoadReader:
    HOST_CPU_LOAD_INFO = 3

    def __init__(self) -> None:
        self.lib = ctypes.CDLL("/usr/lib/libSystem.B.dylib")
        self.lib.mach_host_self.restype = ctypes.c_uint
        self.previous = self._ticks()

    def _ticks(self) -> Tuple[int, int, int, int]:
        values = (ctypes.c_uint * 4)()
        count = ctypes.c_uint(4)
        result = self.lib.host_statistics(self.lib.mach_host_self(),
                                          self.HOST_CPU_LOAD_INFO,
                                          ctypes.byref(values), ctypes.byref(count))
        if result != 0:
            raise RuntimeError(f"host_statistics failed: {result}")
        return tuple(int(value) for value in values)

    def read(self) -> Optional[float]:
        current = self._ticks()
        delta = [current[i] - self.previous[i] for i in range(4)]
        self.previous = current
        total = sum(delta)
        return None if total <= 0 else 100.0 * (total - delta[2]) / total


def memory_snapshot(
    total_bytes: int,
) -> Tuple[Optional[float], Optional[float], Optional[float]]:
    try:
        output = subprocess.check_output(["/usr/bin/vm_stat"], text=True, timeout=2)
        page_match = re.search(r"page size of (\d+) bytes", output)
        page_size = int(page_match.group(1)) if page_match else 4096
        values = {name: int(value.replace(".", "")) for name, value in
                  re.findall(r"^Pages ([^:]+):\s+([0-9.]+)", output, re.MULTILINE)}
        available_pages = (values.get("free", 0) + values.get("inactive", 0) +
                           values.get("speculative", 0))
        used = max(0, total_bytes - available_pages * page_size)
        total_gb = total_bytes / (1024 ** 3)
        used_gb = used / (1024 ** 3)
        return used_gb, total_gb, 100.0 * used / total_bytes
    except (OSError, subprocess.SubprocessError, ValueError, ZeroDivisionError):
        return None, None, None


def disk_load(path: str = "/") -> Optional[float]:
    try:
        values = os.statvfs(path)
        total = values.f_blocks * values.f_frsize
        available = values.f_bavail * values.f_frsize
        return None if total <= 0 else 100.0 * (total - available) / total
    except (OSError, ZeroDivisionError):
        return None


def process_count() -> Optional[int]:
    try:
        output = subprocess.check_output(
            ["/bin/ps", "-A", "-o", "pid="], text=True, timeout=2)
        return sum(1 for line in output.splitlines() if line.strip())
    except (OSError, subprocess.SubprocessError):
        return None


def gpu_load() -> Optional[float]:
    try:
        output = subprocess.check_output(
            ["/usr/sbin/ioreg", "-r", "-d", "1", "-w", "0", "-c",
             "IOAccelerator"], text=True, stderr=subprocess.DEVNULL, timeout=3)
        patterns = [r'"Device Utilization %"\s*=\s*(\d+)',
                    r'"GPU Activity\(%%\)"\s*=\s*(\d+)',
                    r'"Renderer Utilization %"\s*=\s*(\d+)']
        values = [float(match.group(1)) for pattern in patterns
                  for match in re.finditer(pattern, output)]
        return max(values) if values else None
    except (OSError, subprocess.SubprocessError, ValueError):
        return None


class NetworkRateReader:
    def __init__(self) -> None:
        self.interface = self._default_interface()
        self.previous = self._counters()
        self.previous_at = time.monotonic()

    @staticmethod
    def _default_interface() -> Optional[str]:
        try:
            output = subprocess.check_output(
                ["/sbin/route", "-n", "get", "default"], text=True,
                stderr=subprocess.DEVNULL, timeout=2)
            match = re.search(r"interface:\s*(\S+)", output)
            return match.group(1) if match else None
        except (OSError, subprocess.SubprocessError):
            return None

    def _counters(self) -> Optional[Tuple[int, int]]:
        if not self.interface:
            return None
        try:
            output = subprocess.check_output(
                ["/usr/sbin/netstat", "-ibn", "-I", self.interface],
                text=True, stderr=subprocess.DEVNULL, timeout=2)
            lines = [line.split() for line in output.splitlines() if line.strip()]
            header = next((line for line in lines if "Ibytes" in line and
                           "Obytes" in line), None)
            if not header:
                return None
            in_index, out_index = header.index("Ibytes"), header.index("Obytes")
            rows = [line for line in lines if line[0] == self.interface and
                    len(line) > max(in_index, out_index)]
            if not rows:
                return None
            return max(int(line[in_index]) for line in rows), \
                max(int(line[out_index]) for line in rows)
        except (OSError, subprocess.SubprocessError, ValueError, IndexError):
            return None

    def read(self) -> Tuple[Optional[float], Optional[float]]:
        now, current = time.monotonic(), self._counters()
        elapsed = now - self.previous_at
        previous = self.previous
        self.previous, self.previous_at = current, now
        if not current or not previous or elapsed <= 0:
            return None, None
        down = max(0, current[0] - previous[0]) * 8.0 / elapsed / 1_000_000
        up = max(0, current[1] - previous[1]) * 8.0 / elapsed / 1_000_000
        return down, up


@dataclass
class SmcValues:
    cpu_temp: Optional[float] = None
    gpu_temp: Optional[float] = None
    cpu_fan_rpm: Optional[float] = None
    gpu_fan_rpm: Optional[float] = None
    cpu_power_watts: Optional[float] = None
    gpu_power_watts: Optional[float] = None


class SmcReader:
    def __init__(self, enabled: bool) -> None:
        self.native_helper = os.environ.get(
            "PCSCREEN_SMC_HELPER", "/usr/local/libexec/pcscreen_smc")
        self.use_native = os.path.isfile(self.native_helper) and os.access(
            self.native_helper, os.X_OK)
        self.use_powermetrics = enabled and os.geteuid() == 0
        self.enabled = self.use_native or self.use_powermetrics
        self.values = SmcValues()
        self.lock = threading.Lock()
        if self.enabled:
            threading.Thread(target=self._worker, daemon=True).start()

    def _worker(self) -> None:
        while True:
            try:
                if self.use_native:
                    result = subprocess.run([self.native_helper], capture_output=True,
                                            text=True, timeout=3, check=False)
                    payload = json.loads(result.stdout or "{}")
                    fans = payload.get("fans", [])
                    cpu = self._safe_float(payload.get("cpuTemp"))
                    gpu = self._safe_float(payload.get("gpuTemp"))
                    cpu_power = None
                    gpu_power = None
                else:
                    result = subprocess.run(
                        ["/usr/bin/powermetrics", "--samplers", "smc",
                         "--sample-rate", "1000", "--sample-count", "1"],
                        capture_output=True, text=True, timeout=8, check=False)
                    text = result.stdout + "\n" + result.stderr
                    cpu = self._number(text, r"CPU(?: die)? temperature:\s*([0-9.]+)")
                    gpu = self._number(text, r"GPU(?: die)? temperature:\s*([0-9.]+)")
                    fans = re.findall(
                        r"Fan(?: speed)?(?: \d+)?:\s*([0-9.]+)\s*rpm", text,
                        re.IGNORECASE)
                    cpu_power = self._power(text, r"CPU Power:\s*([0-9.]+)\s*(mW|W)")
                    gpu_power = self._power(text, r"GPU Power:\s*([0-9.]+)\s*(mW|W)")
                with self.lock:
                    self.values = SmcValues(
                        cpu, gpu, self._safe_float(fans[0]) if fans else None,
                        self._safe_float(fans[1]) if len(fans) > 1 else None,
                        cpu_power, gpu_power)
            except (OSError, subprocess.SubprocessError, ValueError,
                    json.JSONDecodeError):
                pass
            time.sleep(3)

    @staticmethod
    def _safe_float(value: object) -> Optional[float]:
        try:
            number = float(value)
            return number if number == number else None
        except (TypeError, ValueError):
            return None

    @staticmethod
    def _number(text: str, pattern: str) -> Optional[float]:
        match = re.search(pattern, text, re.IGNORECASE)
        return float(match.group(1)) if match else None

    @staticmethod
    def _power(text: str, pattern: str) -> Optional[float]:
        match = re.search(pattern, text, re.IGNORECASE)
        if not match:
            return None
        value = float(match.group(1))
        return value / 1000.0 if match.group(2).lower() == "mw" else value

    def read(self) -> SmcValues:
        with self.lock:
            return SmcValues(**vars(self.values))


def candidate_ports(requested: Optional[str]) -> List[str]:
    if requested:
        return [requested]
    preferred = []
    for port in list_ports.comports():
        if any(token in port.device for token in ("usbmodem", "usbserial", "wchusbserial")):
            preferred.append(port.device)
    return preferred or sorted(glob.glob("/dev/cu.usb*"))


def connect(requested: Optional[str]) -> serial.Serial:
    while True:
        for device in candidate_ports(requested):
            try:
                # Set DTR/RTS before opening so native USB CDC is not reset by
                # an avoidable modem-control transition.
                port = serial.Serial(port=None, baudrate=115200, timeout=0.10,
                                     write_timeout=1)
                port.dtr = False
                port.rts = False
                port.port = device
                port.open()
                time.sleep(0.35)
                port.reset_input_buffer()
                response = ""
                deadline = time.monotonic() + 2.0
                while time.monotonic() < deadline:
                    port.write(b'{"type":"hello"}\n')
                    for _ in range(4):
                        line = port.readline().decode(errors="replace").strip()
                        if line:
                            response = line
                        if "PCSCREEN-S3" in line:
                            print(f"Da ket noi {device}: {line}")
                            return port
                port.close()
            except (OSError, serial.SerialException, termios.error):
                continue
        print("Chua tim thay PCSCREEN-S3; se thu lai sau 3 giay...")
        time.sleep(3)


def read_device_messages(port: serial.Serial, interval: float,
                         paused: bool) -> Tuple[float, bool, List[str]]:
    """Drain complete firmware messages without stalling telemetry delivery."""
    messages: List[str] = []
    for _ in range(16):
        if port.in_waiting <= 0:
            break
        line = port.readline().decode(errors="replace").strip()
        if not line:
            break
        messages.append(line)
        interval, paused = apply_device_rate(line, interval, paused)
    return interval, paused, messages


def add(packet: Dict[str, object], name: str, value: Optional[float], digits: int = 1) -> None:
    if value is not None:
        packet[name] = round(value, digits)


def main() -> int:
    instance_lock = acquire_single_instance()
    if instance_lock is None:
        print("PCScreen Agent da dang chay. Khong mo them tien trinh thu hai.")
        return 2

    parser = argparse.ArgumentParser()
    parser.add_argument("--port", help="vi du /dev/cu.usbmodem14501")
    parser.add_argument("--interval", type=float, default=1.0)
    parser.add_argument("--enhanced", action="store_true",
                        help="read Intel SMC through root powermetrics")
    args = parser.parse_args()

    total_bytes = int(sysctl("hw.memsize") or 0)
    if total_bytes <= 0:
        try:
            total_bytes = os.sysconf("SC_PAGE_SIZE") * os.sysconf("SC_PHYS_PAGES")
        except (OSError, ValueError):
            total_bytes = 0
    clock_hz = int(sysctl("hw.cpufrequency") or 0)
    core_count = int(sysctl("hw.logicalcpu") or os.cpu_count() or 0)
    boot_text = sysctl("kern.boottime") or ""
    boot_match = re.search(r"sec\s*=\s*(\d+)", boot_text)
    boot_seconds = int(boot_match.group(1)) if boot_match else int(time.time())
    cpu = CpuLoadReader()
    network = NetworkRateReader()
    smc = SmcReader(args.enhanced)
    if smc.use_native:
        sensor_mode = "SMC native (khong can sudo)"
    elif smc.use_powermetrics:
        sensor_mode = "powermetrics root"
    else:
        sensor_mode = "khong co SMC helper"
    print("Che do sensor:", sensor_mode)

    port: Optional[serial.Serial] = None
    last_slow_sample = 0.0
    disk_percent: Optional[float] = None
    processes: Optional[int] = None
    graphics_load: Optional[float] = None
    current_interval = max(0.25, min(5.0, args.interval))
    telemetry_paused = False
    last_hello = 0.0
    try:
        while True:
            if port is None or not port.is_open:
                port = connect(args.port)
                last_hello = 0.0
            try:
                current_interval, telemetry_paused, messages = read_device_messages(
                    port, current_interval, telemetry_paused)
                now = time.monotonic()
                if telemetry_paused or now - last_hello >= 10.0:
                    port.write(b'{"type":"hello"}\n')
                    port.flush()
                    last_hello = now
                    if telemetry_paused:
                        print("Telemetry dang tam dung; cho lenh tiep tuc tu ESP32...")
                        time.sleep(0.5)
                        continue
                cpu_load = cpu.read()
                used_gb, total_gb, memory_load = memory_snapshot(total_bytes)
                sensors = smc.read()
                network_down, network_up = network.read()
                if time.monotonic() - last_slow_sample >= 5.0:
                    disk_percent = disk_load()
                    processes = process_count()
                    graphics_load = gpu_load()
                    last_slow_sample = time.monotonic()
                packet: Dict[str, object] = {"type": "telemetry"}
                add(packet, "cpuLoad", cpu_load)
                add(packet, "cpuClockMHz", clock_hz / 1_000_000 if clock_hz else None, 0)
                add(packet, "cpuTemp", sensors.cpu_temp)
                add(packet, "gpuTemp", sensors.gpu_temp)
                add(packet, "gpuLoad", graphics_load)
                add(packet, "cpuFanRpm", sensors.cpu_fan_rpm, 0)
                add(packet, "gpuFanRpm", sensors.gpu_fan_rpm, 0)
                add(packet, "cpuPowerWatts", sensors.cpu_power_watts)
                add(packet, "gpuPowerWatts", sensors.gpu_power_watts)
                add(packet, "memoryUsedGb", used_gb)
                add(packet, "memoryTotalGb", total_gb)
                add(packet, "ramLoad", memory_load)
                add(packet, "diskLoad", disk_percent)
                add(packet, "networkDownMbps", network_down, 2)
                add(packet, "networkUpMbps", network_up, 2)
                add(packet, "loadAverage", os.getloadavg()[0], 2)
                packet["uptimeSeconds"] = max(0, int(time.time()) - boot_seconds)
                if processes is not None:
                    packet["processCount"] = processes
                packet["cpuCoreCount"] = core_count
                wire = json.dumps(packet, separators=(",", ":"), allow_nan=False) + "\n"
                port.write(wire.encode())
                port.flush()
                status = messages[-1] if messages else "sent"
                print(f"CPU {cpu_load or 0:5.1f}% | RAM {memory_load or 0:5.1f}% | "
                      f"TEMP {sensors.cpu_temp if sensors.cpu_temp is not None else '--'} | "
                      f"FAN {sensors.cpu_fan_rpm if sensors.cpu_fan_rpm is not None else '--'} | "
                      f"DISK {disk_percent or 0:4.1f}% | NET {network_down or 0:.2f}/{network_up or 0:.2f} Mbps | {status}")
            except (OSError, serial.SerialException, termios.error):
                try:
                    port.close()
                except Exception:
                    pass
                port = None
            time.sleep(current_interval)
    except KeyboardInterrupt:
        print("\nDa dung PC Screen Agent.")
    finally:
        if port and port.is_open:
            port.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
