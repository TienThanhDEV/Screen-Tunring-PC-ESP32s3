# PCScreen ESP32-S3 — Public v1.7.0

Màn hình giám sát máy tính dùng **ESP32-S3 Super Mini 4 MB** và **TFT ST7789 1.54 inch 240×240**. Firmware nhận telemetry qua USB CDC, hiển thị các trang tổng quan/nhiệt độ/quạt/hiệu năng, cung cấp bảng điều khiển web, ảnh hoặc GIF khởi động, OTA một tệp và đồng bộ manifest từ GitHub Pages.

**Tác giả:** Nguyễn Tiến Thành  
**GitHub:** [TienThanhDEV](https://github.com/TienThanhDEV)

## Cài nhanh

- Bo chưa từng cài v1.7.0: dùng gói `installers/usb-flash` và nạp file `PCScreen-S3-4MB-v1.7.0-FULL.bin` tại offset `0x0`.
- Bo đã có đúng bảng phân vùng v1.7.0: mở web ESP32 → **Hệ thống → Cập nhật OTA** và chọn `PCScreen-S3-4MB-v1.7.0.pcota`.
- Windows: chạy một ứng dụng PCScreen Windows Agent. Ứng dụng tích hợp LibreHardwareMonitorLib, tự tìm cổng COM và gửi dữ liệu 250–5000 ms.
- macOS Intel: mở `mac-agent/RUN_MAC_AGENT.command`; nếu cần nhiệt độ/quạt, cài helper AppleSMC từ mã nguồn bằng `INSTALL_SMC_HELPER.command`.

> **Bắt buộc nạp USB một lần khi chuyển từ v1.5.x trở xuống.** OTA chỉ thay ứng dụng, không thay bảng phân vùng. Dùng OTA trực tiếp từ bố cục cũ sẽ không tạo được vùng ảnh 1,2 MB mới.

Sau khi nạp mới, kết nối `PCScreen-Setup`, mật khẩu `pcscreen123`, rồi mở `http://192.168.4.1`.

## Màn hình cài đặt lần đầu

Sau khi nạp FULL và xóa flash, TFT hiển thị mã QR Wi-Fi cùng SSID `PCScreen-Setup`, mật khẩu `pcscreen123` và địa chỉ cấu hình. Quét QR hoặc kết nối thủ công, vào trang web và chọn Wi-Fi Internet. Khi kết nối thành công, thiết bị chạy **CHÀO MỪNG** hoặc **WELCOME** theo ngôn ngữ đã chọn rồi mới vào màn hình chính.

Trạng thái này được lưu trong NVS. OTA giữ nguyên NVS nên không hiện lại màn hình cấu hình lần đầu. Khi nâng cấp từ bản cũ đã có SSID, firmware tự nhận diện là thiết bị đã cấu hình và giữ quy trình khởi động bình thường.

Web ESP32, màn hình TFT và Windows Agent hỗ trợ chuyển đổi trọn bộ **Tiếng Việt/English**. Windows Agent lưu ngôn ngữ và tự khởi động lại một lần để đồng bộ toàn bộ cửa sổ, menu khay và thông báo.

## Nối dây

| TFT ST7789 | ESP32-S3 Super Mini |
|---|---:|
| VCC | 3V3 |
| GND | GND |
| SCL/SCK | GPIO13 |
| SDA/MOSI | GPIO12 |
| RST | GPIO11 |
| DC | GPIO10 |
| CS | GPIO9 |
| BL | GPIO8 trực tiếp* |

\* Cấu hình này dành cho module 1.54TFT-SPI-ST7789 Ver 1.1 có transistor đệm BL như hình phần cứng của dự án. Với module khác, phải kiểm tra dòng BL trước khi nối trực tiếp GPIO.

## Bố cục flash 4 MB

| Vùng | Offset | Dung lượng |
|---|---:|---:|
| NVS | `0x9000` | 20 KiB |
| OTA data | `0xE000` | 8 KiB |
| Firmware A | `0x10000` | 1,441,792 byte |
| Firmware B | `0x170000` | 1,441,792 byte |
| LittleFS/ảnh | `0x2D0000` | 1,245,184 byte |

Ảnh/GIF khởi động bị giới hạn chính xác **1.200.000 byte**. Giao diện quản trị được nhúng trong firmware và không chiếm LittleFS; phần chênh còn lại dành cho metadata hệ thống file.

## Cấu trúc repository

- `firmware/`: PlatformIO/C++, LovyanGFX, webserver và OTA.
- `windows-agent/`: WinForms .NET 8, LibreHardwareMonitorLib, system tray.
- `mac-agent/`: Python agent + AppleSMC helper nguồn mở cho macOS Intel.
- `docs/`: GitHub Pages quản lý manifest, hiệu ứng, trang và thiết bị.
- `cloudflare-worker/`: API thiết bị dùng Workers KV.
- `installers/usb-flash/`: bộ nạp USB macOS/Windows.
- `release-assets/`: BIN, PCOTA, checksum và báo cáo build.

## Build

```bash
cd firmware
pio run -e esp32-s3-super-mini-4mb
python3 tools/make_ota.py .pio/build/esp32-s3-super-mini-4mb/firmware.bin ../release-assets/PCScreen-S3-4MB-v1.7.0.pcota
```

Windows Agent được build bằng `windows-agent/BUILD_EXE_WINDOWS.bat`, hoặc tự động bằng GitHub Actions. Xem hướng dẫn riêng trong từng thư mục.

macOS Agent dùng Python 3 có sẵn từ Apple Command Line Tools và pySerial đi kèm, không cần `pip install cryptography` hay tải package từ Internet.

## Những gì đã xác minh

- Firmware PlatformIO build thành công cho ESP32-S3 4 MB.
- Bảng phân vùng không chồng lấn và kết thúc đúng trong 4 MB.
- Gói PCOTA có magic, kích thước payload và SHA-256 hợp lệ.
- JavaScript/JSON/Cloudflare Worker đã qua kiểm tra cú pháp; workflow đã được kiểm tra cấu trúc.
- Mã C# đã qua kiểm tra tĩnh. GitHub Actions build EXE trên Windows; môi trường macOS hiện tại không thể xác minh driver/PawnIO và cảm biến thực tế.
- macOS Agent đã qua `py_compile`, script Zsh đã kiểm tra cú pháp và helper AppleSMC biên dịch sạch bằng Clang `-Werror` thành Mach-O x86_64.

Chưa thể xác minh tự động màu/offset của từng lô TFT, dòng điện chân BL, độ mượt trên bo thật, Wi-Fi thực, AppleSMC có quyền trên đúng model Mac và những sensor mà mainboard/GPU cụ thể công bố. Hãy kiểm tra phần cứng trước khi lắp cố định.

## Bảo mật

Không commit GitHub PAT, Cloudflare token hoặc `ADMIN_KEY`. Nếu một token từng xuất hiện trong chat/log, hãy thu hồi và tạo token mới. Xem [SECURITY.md](SECURITY.md).

Mã nguồn của dự án được phát hành theo giấy phép MIT. Các thư viện bên thứ ba giữ giấy phép riêng; xem [THIRD_PARTY.md](THIRD_PARTY.md).
