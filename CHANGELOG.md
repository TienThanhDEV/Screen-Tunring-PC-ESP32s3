# Changelog

## 1.7.0 — 2026-07-14

- Thêm màn hình cấu hình Wi-Fi lần đầu với QR, SSID, mật khẩu và địa chỉ web.
- Sau khi kết nối Wi-Fi lần đầu, chạy hiệu ứng CHÀO MỪNG/WELCOME rồi vào giao diện chính.
- Lưu trạng thái provisioning trong NVS; cập nhật OTA từ bản cũ giữ nguyên quy trình khởi động thường.
- Đồng bộ chuyển đổi toàn bộ Tiếng Việt/English cho TFT, web cục bộ, GitHub Pages và Windows Agent.
- Windows Agent 1.5.0 có bộ chọn ngôn ngữ và bỏ nhãn ACK gây hiểu nhầm cho telemetry fire-and-forget.
- Bổ sung README tiếng Anh và bộ nạp USB một lần nhấp bằng tiếng Việt/English cho macOS Intel và Windows.

## 1.6.0 — 2026-07-14

- Giới hạn ảnh/GIF khởi động đúng 1.200.000 byte.
- Chia lại flash 4 MB: hai slot OTA 0x160000 và LittleFS 0x130000; loại vùng NVS dự phòng/core dump chưa dùng để tăng dư địa firmware.
- Phục vụ web console trực tiếp từ firmware, không nhân đôi 83 KB trong LittleFS.
- USB boot animation non-blocking; Windows Agent 1.4.0 gửi telemetry không chờ ACK.
- Thêm macOS Intel Agent 1.4.0; USB không chờ ACK, DTR/RTS tắt và helper AppleSMC được biên dịch tại máy rồi hạ quyền vĩnh viễn.
- Cloud request fail-fast hơn để giảm gián đoạn UI/USB.
- Cloudflare Worker v2 kiểm tra payload, CORS chính xác, phân trang KV, endpoint health và observability.
- Bổ sung license, bảo mật, attribution, workflow build/release và bộ nạp USB ổn định 115200 baud.
