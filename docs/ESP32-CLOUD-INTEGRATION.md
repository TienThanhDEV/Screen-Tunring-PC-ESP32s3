# Kết nối ESP32-S3 với PCScreen Cloud

## Endpoint sau khi bật GitHub Pages

```text
https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/data/device-manifest.json
https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/data/effects.json
https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/data/firmware-manifest.json
```

ESP32 chỉ thực hiện `GET` qua HTTPS. Không đưa GitHub token vào firmware.

## Chu kỳ đồng bộ an toàn

- Tải khi Wi-Fi vừa kết nối.
- Sau đó kiểm tra mỗi 15–60 phút, không gọi mỗi vòng `loop()`.
- Giữ cấu hình hợp lệ gần nhất trong NVS/LittleFS nếu Internet mất.
- Chỉ chấp nhận `schemaVersion` mà firmware hiểu.
- Kiểm tra `board`, `flashMB`, kích thước và SHA-256 trước OTA.
- Không tự cài bản cập nhật bắt buộc nếu nguồn điện không ổn định.
# Kết nối ESP32-S3 với PCScreen Cloud

## Endpoint sau khi bật GitHub Pages

```text
https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/data/device-manifest.json
https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/data/effects.json
https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/data/firmware-manifest.json
```

ESP32 chỉ thực hiện `GET` qua HTTPS. Không đưa GitHub token vào firmware.

## Chu kỳ đồng bộ an toàn

- Tải khi Wi-Fi vừa kết nối.
- Sau đó kiểm tra mỗi 15–60 phút, không gọi mỗi vòng `loop()`.
- Giữ cấu hình hợp lệ gần nhất trong NVS/LittleFS nếu Internet mất.
- Chỉ chấp nhận `schemaVersion` mà firmware hiểu.
- Kiểm tra `board`, `flashMB`, kích thước và SHA-256 trước OTA.
- Không tự cài bản cập nhật bắt buộc nếu nguồn điện không ổn định.

## Thử mã mẫu

Chép ba file trong `firmware-example/` vào project PlatformIO/Arduino, cài
`ArduinoJson`, đổi Wi-Fi và URL GitHub Pages rồi mở Serial Monitor 115200.

Mã mẫu dùng `setInsecure()` để bắt đầu nhanh. Bản phát hành thực tế phải cấu
hình chứng thư CA với `WiFiClientSecure::setCACert()` hoặc dùng certificate
bundle được cập nhật, nếu không thiết bị không xác thực danh tính máy chủ.

## Giới hạn

GitHub Pages là host tĩnh: ESP32 có thể tải manifest nhưng không thể gửi
telemetry thời gian thực lên đó. Muốn hiển thị online/offline, nhiệt độ hoặc log
từ xa cần một API nhận `POST` (Cloudflare Worker, Firebase, Supabase hoặc server
riêng) và cơ chế xác thực từng thiết bị.

## Thử mã mẫu

Chép ba file trong `firmware-example/` vào project PlatformIO/Arduino, cài
`ArduinoJson`, đổi Wi-Fi và URL GitHub Pages rồi mở Serial Monitor 115200.

Mã mẫu dùng `setInsecure()` để bắt đầu nhanh. Bản phát hành thực tế phải cấu
hình chứng thư CA với `WiFiClientSecure::setCACert()` hoặc dùng certificate
bundle được cập nhật, nếu không thiết bị không xác thực danh tính máy chủ.

## Giới hạn

GitHub Pages là host tĩnh: ESP32 có thể tải manifest nhưng không thể gửi
telemetry thời gian thực lên đó. Muốn hiển thị online/offline, nhiệt độ hoặc log
từ xa cần một API nhận `POST` (Cloudflare Worker, Firebase, Supabase hoặc server
riêng) và cơ chế xác thực từng thiết bị.
