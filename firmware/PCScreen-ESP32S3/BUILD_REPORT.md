# Báo cáo kiểm tra

Ngày kiểm tra: 2026-07-13.

## Kết quả

| Kiểm tra | Kết quả |
|---|---|
| PlatformIO compile firmware | PASS |
| Tạo `firmware.bin` | PASS |
| Tạo LittleFS image từ HTML/CSS/JS | PASS |
| JavaScript syntax (`node --check`) | PASS |
| Python demo syntax (`py_compile`) | PASS |
| Tạo container OTA một file `.pcota` | PASS |
| Web Bento/NTP render trong trình duyệt | PASS |

## Firmware 0.6.0 — system controls, LittleFS và OTA layout

```text
PlatformIO build 4 MB: PASS
RAM: 73,224 / 327,680 bytes (22.3%)
App flash: 1,021,049 / 1,245,184 bytes (82.0%)
Firmware BIN: 1,021,408 bytes
LittleFS image: 1,572,864 bytes (1.50 MiB)
Asset upload limit: 1,228,800 bytes (1,200 KiB)
JavaScript syntax/DOM IDs: PASS
Web render + console errors: PASS / 0 errors
```

Đã kiểm tra trình duyệt cho tab Màn hình, Hình ảnh và Hệ thống. OTA hiển thị hai vùng độc lập, animation không che phần chọn file/nút cập nhật. Compiler đã xác minh REST restart/system status, lệnh WebSocket và partition CSV mới. Khởi động lại thật, flash full layout mới, upload ảnh 1,2 MB và OTA trên Wi-Fi vẫn cần thử trực tiếp trên ESP32-S3.

## Firmware 0.5.0 — Bento 240×240 và NTP

```text
PlatformIO build 4 MB: PASS
JavaScript syntax/DOM IDs: PASS
Web desktop render: PASS
Dashboard: 2 cột × 3 hàng, 240×240 px
Wi-Fi mode: AP + STA
NTP: 3 máy chủ dự phòng, UTC+7 mặc định
RAM: 73,224 / 327,680 bytes (22.3%)
App flash: 1,006,441 / 1,310,720 bytes (76.8%)
Firmware BIN: 1,006,800 bytes
LittleFS image: 1,441,792 bytes
Full image: 4,128,768 bytes
```

Đã kiểm tra compiler cho `NetworkTimeManager`, REST `/api/v1/network`, lưu NVS, đồng hồ/ngày và cấu hình Bento. Trang web được chạy với API mô phỏng và kiểm tra trực quan ở theme tối. Kết nối Wi-Fi 2,4 GHz, DNS/NTP, độ chính xác thời gian sau mất nguồn và hình TFT thực tế vẫn cần xác minh trên ESP32-S3 của người dùng.

## Firmware 0.3.0 — OTA web

OTA dùng phân vùng `app0/app1` của layout 4 MB. File `.pcota` chứa header nhận dạng 16 byte và image ứng dụng; firmware từ chối file full-flash hoặc file sai định dạng. Cấu hình và LittleFS không bị ghi đè khi OTA.

Đã kiểm tra được ở mức compiler và kiểm tra cấu trúc file. Việc chuyển boot partition, mất điện giữa lúc ghi, kết nối Wi-Fi và rollback vẫn cần xác minh trực tiếp trên board.

## Firmware 0.3.1 — BL trực tiếp

Pin map được khóa thành `GPIO8 → TFT BL` cho module `1.54TFT-SPI-ST7789 Ver:1.1` có tầng transistor/điện trở onboard. PWM vẫn active-high 20 kHz. Việc xác nhận cực tính, độ tuyến tính độ sáng và nhiệt độ transistor onboard cần kiểm tra trên module thật.

## Firmware 0.4.0 — dashboard, RGB effects và WebSocket

```text
PlatformIO build 4 MB: PASS
RAM: 72,960 / 327,680 bytes (22.3%)
App flash: 948,765 / 1,310,720 bytes (72.4%)
JavaScript syntax: PASS
WebSockets library: 2.6.1
```

Đã kiểm tra compile cho 4 trang TFT, telemetry protocol 2, 5 hiệu ứng RGB, setting NVS, REST và WebSocket. Chưa thể đo nhiệt độ thực tế, dòng LED, độ mượt GIF/WebSocket hoặc sensor fan trên phần cứng PC của người dùng.

## Firmware 0.4.1 — OTA animation

```text
PlatformIO build 4 MB: PASS
RAM: 72,992 / 327,680 bytes (22.3%)
App flash: 950,621 / 1,310,720 bytes (72.5%)
Render interval: 100 ms
RGB range: 0–100%, default 25%
OTA TFT callback: throttle 100 ms
```

Animation và state machine OTA đã được xác minh ở mức compiler/luồng mã. Chưa thể quan sát tốc độ frame thực khi upload Wi-Fi hoặc thử tình huống mất nguồn trên board thật.

## Firmware 0.4.2 — crop 240×240 và theme

```text
PlatformIO build 4 MB: PASS
RAM: 72,992 / 327,680 bytes (22.3%)
App flash: 951,353 / 1,310,720 bytes (72.6%)
JavaScript syntax/DOM IDs: PASS
```

Đã kiểm tra compiler cho parser kích thước PNG/JPEG, cover-scale và center-crop GIF. Canvas crop được kiểm tra ở mức cú pháp/DOM; thao tác kéo/zoom và kết quả hình ảnh thực tế vẫn cần thử trong trình duyệt và trên TFT thật.

## Firmware 0.4.3 — logo toàn màn và trang logo

```text
PlatformIO build 4 MB: PASS
RAM: 73,008 / 327,680 bytes (22.3%)
App flash: 970,433 / 1,310,720 bytes (74.0%)
LittleFS image: 1,441,792 bytes
PCOTA payload: 970,800 bytes, header/version/size/magic hợp lệ
Full image: 4,128,768 bytes
```

Đã xác minh compiler cho vị trí ảnh `(0,0)`, state machine GIF không chặn, page mask 5 trang, thời gian logo riêng và cơ chế tự đồng bộ web assets sau OTA. Hình ảnh thực tế, tốc độ GIF và chuyển trang vẫn cần xác minh trực tiếp trên TFT của người dùng.

Môi trường build:

```text
PlatformIO Core 6.1.18
platformio/espressif32 6.9.0
Arduino-ESP32 2.0.17 (PlatformIO package 3.20017)
LovyanGFX 1.2.7
ArduinoJson 7.4.3
Target: esp32-s3-devkitc-1 N8, no PSRAM
```

Kích thước firmware:

```text
RAM:   46,968 / 327,680 bytes (14.3%)
Flash: 881,293 / 3,342,336 bytes (26.4%)
```

Firmware hỗ trợ cả API LEDC Arduino-ESP32 2.x và 3.x bằng kiểm tra phiên bản tại thời điểm biên dịch. Build xác nhận trực tiếp ở nhánh 2.x được PlatformIO 6.9.0 cung cấp.

## Chưa thể xác minh không có thiết bị thật

- Board clone được cấu hình theo flash 4 MB/no-PSRAM; vẫn cần xác nhận chip thật.
- GPIO thực tế có đúng chữ in trên board hay không.
- Cực tính BL và tầng transistor onboard đúng với module Ver:1.1 trong ảnh.
- Màu RGB/BGR, inversion, rotation và offset của panel cụ thể.
- SPI 40 MHz ổn định với dây thực tế.
- USB-C trên board nối native USB CDC hay mạch USB-UART.
- Upload, boot, Wi-Fi, TFT và độ bền chạy dài hạn trên thiết bị.

Những mục này phải được thực hiện theo checklist trong README và `docs/WIRING.md`; build thành công không thay thế kiểm thử điện.

## Firmware 0.3.1 — cấu hình 4 MB

```text
Build firmware: PASS
Build LittleFS: PASS
JavaScript syntax: PASS
RAM: 71,544 / 327,680 bytes (21.8%)
App flash: 924,033 / 1,310,720 bytes (70.5%)
```

Đã bổ sung AnimatedGIF 2.2.3 và Adafruit NeoPixel 1.15.5. Upload ảnh/GIF và RGB đã được xác minh ở mức compile; phát GIF, GPIO48 và upload trên mạng thật vẫn cần kiểm thử trên board.
