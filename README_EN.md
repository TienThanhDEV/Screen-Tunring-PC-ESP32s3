# PCScreen ESP32-S3 — Public v1.7.0

PC hardware monitor for an **ESP32-S3 Super Mini with 4 MB flash** and a **1.54-inch 240×240 ST7789 TFT**. The firmware receives telemetry over USB CDC, renders overview/temperature/cooling/performance pages, provides a bilingual local web console, supports a PNG/JPG/GIF startup image, installs a single-file OTA package, and synchronizes public data from GitHub Pages.

**Developer:** Nguyễn Tiến Thành  
**GitHub:** [TienThanhDEV](https://github.com/TienThanhDEV)

## First boot

A clean USB installation erases NVS and starts the one-time provisioning flow:

1. The TFT shows a QR code, setup SSID `PCScreen-Setup`, password `pcscreen123`, and the setup address.
2. Scan the QR code or connect manually, then open `http://192.168.4.1`.
3. Select a Wi-Fi network in **Connectivity & time**, enter its password, and save.
4. When the station connection succeeds, the TFT shows **WELCOME** and enters the normal page rotation.

The provisioning marker is stored in NVS. A normal OTA update preserves it, so OTA reboots follow the standard startup-image flow and do not show provisioning again.

## One-click installation

- macOS Intel: open `installers/usb-flash/FLASH_ESP32S3_4MB_MAC.command` (or the Vietnamese `NAP_ESP32S3_4MB_MAC.command`).
- Windows: open `installers/usb-flash/FLASH_ESP32S3_4MB_WINDOWS.bat` (or the Vietnamese `NAP_ESP32S3_4MB_WINDOWS.bat`).
- Existing v1.6.0 installation with the same partition table: open the ESP32 web console, go to **System → Firmware update**, and upload `PCScreen-S3-4MB-v1.7.0.pcota`.

Use the FULL USB image when migrating from v1.5.x or older because OTA cannot replace the partition table.

## Wiring

| ST7789 TFT | ESP32-S3 Super Mini |
|---|---:|
| VCC | 3V3 |
| GND | GND |
| SCL/SCK | GPIO13 |
| SDA/MOSI | GPIO12 |
| RST | GPIO11 |
| DC | GPIO10 |
| CS | GPIO9 |
| BL | GPIO8 directly* |

\* Direct BL control targets the `1.54TFT-SPI-ST7789 Ver 1.1` module with an on-board buffer transistor. Verify the BL circuit before using a different module.

## 4 MB flash layout

| Region | Offset | Size |
|---|---:|---:|
| NVS | `0x9000` | 20 KiB |
| OTA data | `0xE000` | 8 KiB |
| Firmware A | `0x10000` | 1,441,792 bytes |
| Firmware B | `0x170000` | 1,441,792 bytes |
| LittleFS/startup image | `0x2D0000` | 1,245,184 bytes |

The startup image is limited to exactly **1,200,000 bytes**. Web assets are embedded in the application image, leaving LittleFS for the uploaded image and filesystem metadata.

## Repository layout

- `firmware/`: PlatformIO C++, LovyanGFX, USB CDC, web console, and OTA.
- `windows-agent/`: .NET 8 WinForms app with LibreHardwareMonitorLib and tray mode.
- `mac-agent/`: Python telemetry agent and optional open-source AppleSMC helper for Intel Macs.
- `docs/`: GitHub Pages control console and public manifests.
- `cloudflare-worker/`: optional Workers KV device API.
- `installers/usb-flash/`: one-click macOS and Windows USB installers.
- `release-assets/`: BIN, PCOTA, checksums, and build report.

## Build

```bash
cd firmware
pio run -e esp32-s3-super-mini-4mb
python3 tools/make_ota.py .pio/build/esp32-s3-super-mini-4mb/firmware.bin ../release-assets/PCScreen-S3-4MB-v1.7.0.pcota
```

Build the Windows Agent with `windows-agent/BUILD_EXE_WINDOWS.bat` or let GitHub Actions produce the self-contained x64 EXE package. The macOS agent includes its serial runtime and does not require the obsolete `cryptography` build path.

## Language consistency

Choose **Vietnamese** or **English** in the ESP32 web console. The TFT pages and web console switch as a complete language set. The Windows Agent has its own language selector; changing it saves the preference and restarts the app once so all controls, tray commands, notifications, and errors use the selected language.

## Verification limits

Automated checks cover firmware compilation, flash partition bounds, PCOTA metadata, JavaScript syntax, JSON manifests, Windows source/build workflow, and macOS script syntax. Hardware-only details—including TFT batch-specific color/offset, BL current, real Wi-Fi behavior, USB cable stability, PawnIO access, AppleSMC permissions, and sensors exposed by a particular CPU/GPU/mainboard—must be verified on the target device.

## Security

Never commit GitHub PATs, Cloudflare API tokens, or `ADMIN_KEY`. Revoke any credential that has appeared in a chat, screenshot, or log. See [SECURITY.md](SECURITY.md).

This project is licensed under MIT. Third-party libraries retain their own licenses; see [THIRD_PARTY.md](THIRD_PARTY.md).
