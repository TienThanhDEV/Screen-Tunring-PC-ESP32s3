# Build report — PCScreen v1.7.0

Tác giả: **Nguyễn Tiến Thành** — GitHub **TienThanhDEV**  
Build time: 2026-07-14 (Asia/Ho_Chi_Minh)

## Firmware

- Target: `esp32-s3-super-mini-4mb`
- PlatformIO Core: 6.1.18
- Espressif32 platform: 6.9.0
- Framework Arduino-ESP32: 2.0.17
- Kết quả: **SUCCESS**
- RAM: 75.896 / 327.680 byte (**23,2%**)
- Application flash: 1.364.601 / 1.441.792 byte (**94,6%**)
- `firmware.bin`: 1.364.960 byte
- PCOTA: 1.364.976 byte
- Full USB BIN: 1.430.496 byte

## Phân vùng

`gen_esp32part.py` đọc lại `partitions.bin` thành công. Kiểm tra tự động xác nhận không chồng lấn, vùng cuối kết thúc tại `0x400000`, hai OTA slot đều `0x160000`, LittleFS `0x130000`, và giới hạn ảnh 1.200.000 byte nhỏ hơn phân vùng.

## Kiểm tra tĩnh

- PCOTA magic/version/payload: đạt.
- SHA-256 cho toàn bộ binary: đạt.
- QR provisioning dùng LovyanGFX và firmware đã liên kết thành công trong slot OTA.
- NVS migration giữ quy trình khởi động bình thường cho thiết bị nâng cấp OTA có Wi-Fi đã lưu.
- HTTPS dùng Mozilla CA bundle 53 KiB; không dùng chế độ TLS không xác minh.
- JavaScript `node --check`: xem `VALIDATION-REPORT.md`.
- JSON parse: xem `VALIDATION-REPORT.md`.
- Cloudflare Worker dry-run: ghi riêng theo môi trường; không deploy trong quá trình đóng gói.
- Windows Agent: kiểm tra source tĩnh trên macOS; EXE được build/kiểm tra trong GitHub Actions `windows-latest`.
- macOS Intel Agent: Python/Zsh đạt kiểm tra cú pháp; AppleSMC helper biên dịch sạch bằng Clang `-Werror` thành Mach-O x86_64. Gói public không chứa binary đặc quyền dựng sẵn.

## Chưa thể xác minh không có phần cứng

- Màu, inversion, offset và rotation của từng lô ST7789.
- BL trực tiếp GPIO8 trên module khác profile Ver 1.1.
- Nhiệt độ, độ ổn định USB/Wi-Fi và animation trên bo thật.
- Sensor CPU/GPU/fan mà từng máy Windows và driver công bố.
- Quyền AppleSMC và sensor thực tế trên từng model Mac Intel.
