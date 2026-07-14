#!/usr/bin/env python3
"""Generate compact 1-bit Vietnamese phrase bitmaps for the 240x240 TFT."""

from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "firmware" / "include" / "VietnameseBitmaps.h"
BOLD = Path("/System/Library/Fonts/Supplemental/Arial Bold.ttf")
REGULAR = Path("/System/Library/Fonts/Supplemental/Arial.ttf")

PHRASES = [
    ("Overview", "TỔNG QUAN", BOLD, 16),
    ("Temperature", "NHIỆT ĐỘ", BOLD, 16),
    ("Cooling", "LÀM MÁT", BOLD, 16),
    ("Performance", "HIỆU NĂNG", BOLD, 16),
    ("System", "HỆ THỐNG", BOLD, 16),
    ("Updating", "Đang cập nhật", BOLD, 18),
    ("DoNotPowerOff", "Không ngắt nguồn thiết bị", REGULAR, 11),
    ("Verifying", "Đang kiểm tra", BOLD, 18),
    ("VerifyFirmware", "Xác minh firmware", REGULAR, 11),
    ("Complete", "Hoàn tất", BOLD, 18),
    ("Restarting", "Đang khởi động lại", REGULAR, 11),
    ("UpdateFailed", "Cập nhật lỗi", BOLD, 18),
    ("RetryWebsite", "Thử lại trên website", REGULAR, 11),
    ("ConnectWifi", "KẾT NỐI WI-FI", BOLD, 16),
    ("ScanQr", "Quét mã QR", BOLD, 13),
    ("Welcome", "CHÀO MỪNG", BOLD, 22),
]


def bitmap(text: str, font_path: Path, size: int):
    font = ImageFont.truetype(str(font_path), size)
    left, top, right, bottom = font.getbbox(text)
    width, height = right - left + 2, bottom - top + 2
    image = Image.new("L", (width, height), 0)
    ImageDraw.Draw(image).text((1 - left, 1 - top), text, font=font, fill=255)
    packed = []
    for y in range(height):
        for byte_x in range((width + 7) // 8):
            value = 0
            for bit in range(8):
                x = byte_x * 8 + bit
                if x < width and image.getpixel((x, y)) >= 96:
                    value |= 1 << bit
            packed.append(value)
    return width, height, packed


def main():
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        "namespace ViText {",
        "struct Bitmap { const uint8_t* data; uint16_t width; uint8_t height; };",
        "",
    ]
    metadata = []
    for name, text, font_path, size in PHRASES:
        width, height, data = bitmap(text, font_path, size)
        lines.append(f"static const uint8_t k{name}Data[] PROGMEM = {{")
        for start in range(0, len(data), 16):
            chunk = ", ".join(f"0x{value:02X}" for value in data[start:start + 16])
            lines.append(f"  {chunk},")
        lines.append("};")
        metadata.append((name, width, height))
    lines.append("")
    for name, width, height in metadata:
        lines.append(f"static constexpr Bitmap k{name}{{k{name}Data, {width}, {height}}};")
    lines.extend(["}  // namespace ViText", ""])
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {OUTPUT} ({OUTPUT.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
