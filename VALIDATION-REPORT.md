# Validation report — Public v1.7.0

Ngày kiểm tra: 2026-07-14  
Tác giả: **Nguyễn Tiến Thành** — **TienThanhDEV**

| Hạng mục | Kết quả | Ghi chú |
|---|---|---|
| PlatformIO firmware 4 MB | PASS | RAM 23,2%; app flash 94,6% |
| Partition CSV/BIN | PASS | Không overlap; kết thúc `0x400000` |
| PCOTA header/payload | PASS | Magic `PCSOTA1`, version 1, payload trùng firmware.bin |
| SHA-256 release assets | PASS | `tools/verify_release.py` |
| Firmware web JS | PASS | `node --check` |
| GitHub Pages JS | PASS | `node --check` |
| HTML ID/local assets | PASS | Không ID trùng, DOM reference hợp lệ |
| JSON manifests | PASS | Tất cả parse hợp lệ |
| Cloudflare Worker JS | PASS | `node --check` |
| Wrangler 4.110 dry-run | PASS | Bundle 5,71 KiB; KV/vars nhận đúng; không deploy |
| Secret scan | PASS | Không tìm thấy PAT/API token/private key thật |
| Windows C# static | PASS | XML và brace balance; icon tồn tại |
| Windows EXE runtime | PENDING CI/HARDWARE | Máy build hiện tại không có .NET SDK/Windows/PawnIO |
| macOS Agent Python/Zsh | PASS | `py_compile`; tất cả `.command` qua `zsh -n` |
| AppleSMC helper x86_64 | PASS BUILD | Clang `-Wall -Wextra -Werror`; Mach-O x86_64 |
| AppleSMC sensor runtime | PENDING HARDWARE | Cần cài root helper và thử trên đúng model Mac Intel |
| ESP32/TFT/BL/Wi-Fi thực | PENDING HARDWARE | Cần nạp và soak test trên đúng board/module |

## Sửa lỗi/điểm tối ưu đã áp dụng

- Boot animation không chặn USB CDC.
- Cài USB sạch hiện QR/SSID/mật khẩu; sau khi Wi-Fi kết nối mới chạy CHÀO MỪNG/WELCOME.
- Marker provisioning trong NVS có migration từ SSID cũ nên OTA không hiện lại màn hình cài lần đầu.
- TFT, web ESP32, GitHub Pages và Windows Agent có chuyển đổi trọn bộ Tiếng Việt/English.
- Windows telemetry không chờ ACK; lệnh điều khiển vẫn chờ ACK.
- macOS telemetry không chờ ACK, tắt DTR/RTS trước khi mở USB và dùng helper AppleSMC biên dịch tại máy.
- Cloud request dùng timeout ngắn cho manifest, chỉ chạy sau NTP và xác minh TLS bằng CA bundle.
- Web console chạy từ app flash; LittleFS không còn lưu bản sao 83 KB.
- Upload ảnh/GIF giới hạn đúng 1.200.000 byte.
- Loại phân vùng NVS dự phòng/core dump chưa dùng; tăng mỗi slot OTA thêm 64 KiB, căn đúng 64 KiB và giữ nguyên vùng ảnh.
- Worker lọc trường dữ liệu, giới hạn 8 KiB, CORS exact-origin, phân trang KV và bảo vệ danh sách bằng secret.
- Bộ nạp USB dùng tốc độ ổn định 115200 baud và xác nhận flash 4 MB trước khi xóa.

## Điều kiện phát hành

Chuyển từ v1.5.x trở xuống phải nạp `FULL.bin` qua USB vì PCOTA không thay bảng phân vùng. PCOTA v1.7.0 lớn hơn slot cũ nên firmware cũ sẽ từ chối an toàn thay vì ghi đè phân vùng.
