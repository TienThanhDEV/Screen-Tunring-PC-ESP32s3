# Firmware PCScreen 1.7.0

Target mặc định: ESP32-S3 Super Mini flash 4 MB, TFT ST7789 240×240, USB CDC native.

## Pin mặc định

`SCK=13`, `MOSI=12`, `RST=11`, `DC=10`, `CS=9`, `BL=8`. BL dùng PWM trực tiếp vì profile phần cứng mục tiêu có transistor đệm. RGB onboard dùng profile trong `include/BoardConfig.h`.

## Build và OTA

```bash
pio run -e esp32-s3-super-mini-4mb
python3 tools/make_ota.py \
  .pio/build/esp32-s3-super-mini-4mb/firmware.bin \
  PCScreen-S3-4MB-v1.7.0.pcota
```

Giới hạn OTA là `0x160000` byte. Ảnh/GIF khởi động tối đa 1.200.000 byte. Web console được phục vụ trực tiếp từ image ứng dụng; LittleFS chỉ lưu tài nguyên do người dùng tải lên.

Kết nối HTTPS xác minh máy chủ bằng bundle CA Mozilla nhúng tại `data/cert/x509_crt_bundle.bin`; firmware không dùng `setInsecure()` cho manifest, telemetry cloud hoặc OTA.

## Lưu ý nâng cấp

Bảng phân vùng v1.7.0 khác v1.5.x. Nạp `FULL.bin` qua USB một lần để cài bảng phân vùng mới. Những lần sau có thể dùng `.pcota`.

## Giao thức USB dòng JSON

- PC → ESP: `{"type":"hello"}`.
- ESP → PC: phản hồi gồm `device`, `firmware`, `protocol`, `author`, `github`.
- PC → ESP: một JSON telemetry trên mỗi dòng (`type=telemetry`).
- ESP phản hồi ACK, nhưng PC Agent v1.5.0 không chờ ACK telemetry; lệnh điều khiển vẫn yêu cầu ACK.

Tác giả: **Nguyễn Tiến Thành** — GitHub **TienThanhDEV**.
