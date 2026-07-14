#!/usr/bin/env python3
"""Generate the deterministic PCScreen Windows PNG/ICO application icon."""

from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "PCScreen.WindowsAgent" / "Assets"
SIZE = 1024


def main() -> None:
    ASSETS.mkdir(parents=True, exist_ok=True)
    image = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    # Dark AMD-inspired tile, monitor body and telemetry pulse.
    draw.rounded_rectangle((58, 58, 966, 966), radius=210,
                           fill=(7, 8, 10, 255),
                           outline=(237, 28, 36, 255), width=30)
    draw.rounded_rectangle((172, 210, 852, 706), radius=72,
                           fill=(18, 19, 23, 255),
                           outline=(238, 240, 244, 255), width=26)
    draw.rounded_rectangle((218, 256, 806, 660), radius=38,
                           fill=(3, 16, 20, 255))

    pulse = [(258, 478), (350, 478), (404, 376), (480, 584),
             (554, 318), (626, 478), (764, 478)]
    draw.line(pulse, fill=(54, 224, 208, 255), width=34, joint="curve")
    for x, y in (pulse[0], pulse[-1]):
        draw.ellipse((x - 18, y - 18, x + 18, y + 18),
                     fill=(237, 28, 36, 255))

    draw.rounded_rectangle((436, 704, 588, 786), radius=20,
                           fill=(238, 240, 244, 255))
    draw.rounded_rectangle((316, 778, 708, 842), radius=30,
                           fill=(238, 240, 244, 255))

    png_path = ASSETS / "pcscreen-icon.png"
    ico_path = ASSETS / "pcscreen.ico"
    image.save(png_path, optimize=True)
    image.save(ico_path, format="ICO",
               sizes=[(16, 16), (20, 20), (24, 24), (32, 32),
                      (40, 40), (48, 48), (64, 64), (128, 128),
                      (256, 256)])
    print(png_path)
    print(ico_path)


if __name__ == "__main__":
    main()
