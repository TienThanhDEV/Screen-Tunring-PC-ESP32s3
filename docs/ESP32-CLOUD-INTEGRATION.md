# Kết nối ESP32-S3 với PCScreen Cloud

## GitHub Pages

Firmware đọc các manifest công khai tại:

```text
https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/data/device-manifest.json
https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/data/effects.json
https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/data/firmware-manifest.json
https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/data/pages.json
```

Không đưa GitHub token vào ESP32. Token chỉ được nhập tạm trong trang admin khi cần commit JSON và không được lưu vào localStorage.

## An toàn và chu kỳ đồng bộ

- Chờ Wi-Fi và đồng hồ NTP hợp lệ trước khi mở TLS.
- Xác minh HTTPS bằng Mozilla CA bundle nhúng trong firmware; không dùng `setInsecure()`.
- Kiểm tra theo chu kỳ tối thiểu 15 phút thay vì mỗi vòng lặp.
- Chỉ chấp nhận schema, board, flash size, URL HTTPS, kích thước và SHA-256 hợp lệ.
- PCOTA chỉ cập nhật application. Thay đổi bảng phân vùng yêu cầu FULL.bin qua USB.

## Telemetry thiết bị

GitHub Pages là host tĩnh nên không nhận POST. Backend tùy chọn trong `cloudflare-worker/` nhận heartbeat đã giới hạn kích thước và lưu bản ghi đã lọc vào Workers KV. Danh sách thiết bị yêu cầu `ADMIN_KEY` được lưu bằng `wrangler secret`.

Device ID/MAC hỗ trợ nhận diện, không phải xác thực mật mã. Nếu triển khai riêng tư, tắt `ALLOW_ANONYMOUS_REPORTS` và thêm khóa riêng theo thiết bị.

Tác giả: **Nguyễn Tiến Thành** — GitHub **TienThanhDEV**.
