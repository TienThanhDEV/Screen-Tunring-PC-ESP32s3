# PC Screen ESP32-S3 + ST7789

Firmware mẫu có cấu trúc module cho ESP32-S3 Super Mini flash 4 MB và TFT ST7789 1,54 inch 240×240. Có dashboard TFT, USB CDC JSON-lines, web dashboard, PWM backlight GPIO8, LED RGB onboard, upload hình khởi động, OTA một file và quản lý nhiều thiết bị qua GitHub Pages.

## Firmware 0.7.1 — màn hình tiến trình OTA 240×240

- Thiết kế OTA mới theo mẫu: nền tối, khung bo tròn, vòng phần trăm xanh và thanh tiến trình.
- Hiển thị riêng các bước nhận firmware, xác minh, hoàn tất và thất bại.
- Giữ nguyên dashboard, ảnh/GIF, RGB, NTP, Cloud Fleet và USB telemetry.
- File cập nhật một-tệp: `PCScreen-S3-4MB-v0.7.1-OTA-SCREEN.pcota`.

## Firmware 0.7.0 — Cloud Fleet theo MAC và OTA Internet

- Mỗi ESP32-S3 tự tạo Device ID ổn định dạng `PCSCREEN-AABBCCDDEEFF` từ MAC Wi-Fi station.
- Sau khi người dùng nhập Wi-Fi Internet một lần, board tự đọc control manifest, registry thiết bị và firmware manifest công khai từ GitHub Pages mỗi 15 phút.
- ESP32 không chứa GitHub token và không cần đăng nhập GitHub; token chỉ dùng trong Cloud Console khi admin muốn ghi dữ liệu.
- Tab **Cloud & thiết bị** hiển thị MAC, Device ID, trạng thái đăng ký, phiên bản hiện tại/mới nhất và nút kiểm tra/cập nhật.
- OTA Internet kiểm tra đúng ESP32-S3, flash 4 MB, container `.pcota`, kích thước và SHA-256 trước khi kích hoạt phân vùng mới.
- Có animation OTA trên TFT và vùng trạng thái riêng trên web trong khi tải từ GitHub Release.
- Thiết bị chưa có trong registry vẫn có thể tự liên kết khi `autoProvision` đang bật; admin có thể thêm MAC, khóa/mở thiết bị trên Cloud Console.

Máy chủ mặc định:

```text
https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/data
```

GitHub Pages chỉ là máy chủ tĩnh chỉ đọc đối với ESP32. Nó không thể nhận heartbeat, telemetry hoặc tự ghi thiết bị mới. Registry theo MAC do admin xuất bản; trạng thái trực tiếp vẫn xem tại website cục bộ của từng ESP hoặc qua PC Agent.

## Firmware 0.6.0 — điều khiển hệ thống, ảnh lớn hơn và OTA mới

- Thêm nút **Khởi động lại ESP32** có xác nhận; ưu tiên gửi lệnh qua WebSocket và tự dùng REST khi WebSocket offline.
- Thêm lệnh WebSocket `display`, `brightness` và `restart`, kèm phản hồi `commandAck`.
- Bật/tắt màn hình ở cả tab Màn hình và Hệ thống; thêm preset sáng 25%, 50%, 75% và 100%.
- WebSocket realtime bổ sung trạng thái màn hình, độ sáng, Wi-Fi/RSSI, thời gian, ngày, heap và dung lượng LittleFS.
- Chữ tiêu đề, nhãn, số liệu quan trọng được in đậm hơn trên web; nhãn thẻ và ngày trên TFT được tăng nét.
- Giới hạn PNG/JPG/GIF tăng từ 1 MiB lên 1.200 KiB.
- Layout 4 MB mới tăng LittleFS từ `0x160000` lên `0x180000` (1,50 MiB) và vẫn giữ hai phân vùng OTA `0x130000`.
- Khu OTA chia riêng vùng chọn file và vùng animation, không dùng overlay nên không chèn lên nút hoặc tiến trình.
- Tab Hệ thống mới hiển thị firmware, uptime, flash, LittleFS, heap và trạng thái màn hình.

Để nhận phân vùng LittleFS 1,50 MiB phải nạp `full.bin` một lần bằng USB. Nâng cấp bằng `.pcota` vẫn nhận đủ tính năng 0.6.0 nhưng giữ layout phân vùng cũ; giới hạn ảnh 1.200 KiB vẫn phù hợp với LittleFS cũ.

## Firmware 0.5.0 — giao diện Bento và giờ Internet

- Trang tổng quan TFT mới theo phong cách Bento 2 cột × 3 hàng, tối ưu đúng 240×240 px.
- Sáu thẻ CPU temperature, GPU temperature, RAM, FPS, CPU fan và GPU fan có màu riêng.
- Website có bản xem trước 240×240 và cho đổi trực tiếp tên đầu trang, bán kính bo góc, sáu màu thẻ, ngày tháng và theme sáng/tối.
- ESP32 chạy đồng thời AP `PCScreen-Setup` và Wi-Fi Internet 2,4 GHz; cấu hình SSID/mật khẩu ngay ở tab **Internet & giờ**.
- Đồng bộ NTP từ ba máy chủ dự phòng, mặc định múi giờ Việt Nam UTC+7; đồng hồ và ngày được hiển thị trên TFT.
- SSID, mật khẩu, múi giờ và tùy chỉnh giao diện được lưu trong NVS sau khi mất điện.
- OTA 0.5.0 tự cập nhật toàn bộ web assets mới mà không xóa ảnh/logo người dùng.

### Thiết lập Internet và thời gian

1. Kết nối Mac/điện thoại vào Wi-Fi `PCScreen-Setup`, mật khẩu `pcscreen123`.
2. Mở `http://192.168.4.1`, chọn tab **Internet & giờ**.
3. Nhập Wi-Fi 2,4 GHz của router hoặc hotspot, chọn **Việt Nam UTC+7**, rồi nhấn **Lưu và kết nối**.
4. Chờ trạng thái `Internet: đã kết nối` và `NTP: đã đồng bộ`. AP cấu hình vẫn hoạt động tại `192.168.4.1`.

ESP32-S3 không kết nối Wi-Fi 5 GHz. Thời gian cần Internet để đồng bộ sau mỗi lần mất nguồn; telemetry từ Mac vẫn truyền qua USB CDC độc lập với NTP.

## Firmware 0.4.3 — sửa căn logo và luân phiên logo

- Sửa tọa độ vẽ PNG/JPG từ `(120,120)` về góc vùng vẽ `(0,0)`, loại bỏ lỗi chỉ hiện một phần tư ảnh ở góc dưới-phải.
- Center-crop phủ đúng toàn bộ TFT 240×240 cho cả ảnh cũ và ảnh đã crop trên web.
- Thêm trang thứ 5 **Logo khởi động**, có checkbox bật/tắt trong vòng luân phiên.
- Thời gian trang logo đặt riêng từ 2–30 giây.
- GIF phát từng frame không chặn USB CDC và webserver trong khi trang logo đang mở.
- OTA 0.4.3 tự đồng bộ HTML/JavaScript mới sang LittleFS nhưng giữ nguyên logo người dùng.

## Firmware 0.4.2 — crop ảnh và theme chuẩn

- Web có canvas crop đúng 240×240 px, zoom 100–300%, thanh căn ngang/dọc và kéo ảnh bằng chuột/cảm ứng.
- Ảnh PNG/JPG được xuất thành PNG 240×240 trước khi upload.
- ESP32 đọc kích thước PNG/JPEG và scale kiểu cover để ảnh cũ cũng lấp đầy màn hình.
- Sửa GIF lớn hơn 240 px: dịch đúng vị trí nguồn rồi center-crop từng dòng, giữ nguyên animation.
- Theme sáng dùng nền trắng `#FFFFFF`; theme tối dùng nền đen `#000000` trên web và TFT.
- GIF không bị canvas chuyển thành ảnh tĩnh; file gốc được upload và crop khi phát.

## Firmware 0.4.1 — OTA animation và giá trị bình thường

- TFT hiển thị vòng tròn quay, phần trăm, thanh tiến trình, bước xác minh và trạng thái hoàn tất/lỗi khi OTA.
- Callback vẽ được gọi ngay trong luồng upload và throttle 100 ms để animation vẫn chạy khi nhận file.
- Web có spinner/progress animation đồng bộ với tiến trình upload.
- Render dashboard trở lại 100 ms như trước.
- RGB trở lại dải 0–100%, mặc định 25%; không còn giới hạn 35%.
- Bỏ ép Wi-Fi sleep và bỏ yield bổ sung trong vòng lặp chính.
- Các trang TFT, theme, WebSocket và hiệu ứng RGB của 0.4.0 vẫn được giữ.

## Firmware 0.4.0 — dashboard nâng cao

- TFT tự luân phiên 4 trang: tổng quan, nhiệt độ, quạt và hiệu năng.
- Telemetry mới: CPU/GPU fan RPM, clock, power, VRAM, RAM used/total và FPS.
- RGB: màu tĩnh, nhịp thở, cầu vồng, màu theo nhiệt độ và màu theo tải.
- Web chia 5 tab, theme sáng/tối và realtime qua WebSocket port 81.
- Setting bật/tắt trang, thời gian chuyển 2–30 giây và thời gian phát ảnh khởi động.
- PNG/JPG/GIF động tối đa 1 MB.
- Bản 0.4.0 từng dùng TFT 4 FPS/Wi-Fi sleep/RGB 35%; các giới hạn này đã được hoàn tác ở 0.4.1 theo yêu cầu.
- Bộ cài full BIN riêng cho macOS Intel và Windows.

## Tính năng mới trong firmware 0.3.0

- Nâng cấp firmware qua trang web bằng một file duy nhất có đuôi `.pcota`.
- Kiểm tra chữ ký container, phiên bản, kích thước và chữ ký ESP32 trước khi ghi.
- Ghi trực tiếp vào phân vùng OTA dự phòng rồi tự khởi động lại.
- Giữ nguyên Wi-Fi, cấu hình NVS và hình khởi động trong LittleFS.
- Thanh tiến trình và thông báo lỗi trên dashboard.

### Cập nhật phần cứng firmware 0.3.1

Với đúng module trong ảnh, mặt sau ghi `1.54TFT-SPI-ST7789 Ver:1.1`, chân BL đã có tầng transistor/điện trở trên board màn hình. Nối trực tiếp chân tín hiệu:

```text
ESP32-S3 GPIO8 → TFT BL
```

Firmware xuất PWM active-high 20 kHz trên GPIO8, vẫn hỗ trợ bật/tắt và độ sáng 0–100%. Không cần AO3401A/2N7002 rời cho đúng revision module này.

### Cách nâng cấp OTA

1. Kết nối Wi-Fi `PCScreen-Setup`, mở `http://192.168.4.1`.
2. Tại **Nâng cấp firmware OTA**, chọn file `PCScreen-S3-4MB-v0.6.0.pcota`.
3. Nhấn **Cập nhật và khởi động lại**; giữ nguồn ổn định đến khi board khởi động xong.

Không tải `PCScreen-S3-4MB-full.bin` vào mục OTA. File full BIN chỉ dùng cho bộ nạp USB lần đầu. Nếu tự build, tạo file OTA bằng:

```bash
python3 tools/make_ota.py firmware/.pio/build/esp32-s3-super-mini-4mb/firmware.bin update.pcota
```

## Tính năng mới trong firmware 0.2.0

- Upload hình khởi động PNG, JPG hoặc GIF từ web, tối đa 1 MB.
- Kiểm tra phần mở rộng và chữ ký file trước khi lưu.
- GIF động được giải mã theo từng dòng bằng AnimatedGIF.
- Bật/tắt, chọn màu và độ sáng LED RGB onboard.
- RGB mặc định dùng GPIO48 với một WS2812/SK6812.
- Trạng thái RGB được lưu sau khi mất điện.

Một số ESP32-S3 Super Mini clone dùng chân RGB khác hoặc dùng chung GPIO48 với LED đỏ. Nếu RGB không hoạt động đúng, sửa `RGB_LED_PIN` trong `include/BoardConfig.h` sau khi xác minh sơ đồ board.

## Hai cách bắt đầu

### Cách 1 — Easy Start (khuyến nghị để kiểm tra phần cứng)

Mở `easy-start/EasyStart.ino` bằng Arduino IDE, cài LovyanGFX và chọn:

```text
Board: ESP32S3 Dev Module
USB CDC On Boot: Enabled
USB Mode: Hardware CDC and JTAG
CPU Frequency: 240 MHz
```

Sketch chỉ kiểm tra TFT và PWM BL. Gửi số `0` đến `100` trong Serial Monitor để đổi độ sáng. Hãy đạt bước này trước khi dùng firmware đầy đủ.

### Cách 2 — Firmware đầy đủ bằng PlatformIO

1. Cài VS Code và extension PlatformIO IDE.
2. Mở thư mục `firmware` (không mở thư mục cha).
3. Nếu board có flash 4 MB, sửa/xóa hai dòng cấu hình flash 8 MB trong `platformio.ini` và chọn partition tương ứng.
4. Sửa Wi-Fi trong `include/AppConfig.h`, hoặc để trống để dùng AP mặc định.
5. Build và upload firmware.
6. Upload LittleFS bằng task **Upload Filesystem Image**.
7. Mở Serial Monitor 115200.

Lệnh CLI tương đương:

```bash
pio run
pio run -t upload
pio run -t uploadfs
pio device monitor
```

Khi không có Wi-Fi đã cấu hình, ESP32 phát:

```text
SSID: PCScreen-Setup
Password: pcscreen123
Web: http://192.168.4.1
```

Đổi mật khẩu AP trước khi sử dụng lâu dài.

## Đấu dây

```text
GPIO13 → SCL/SCK
GPIO12 → SDA/MOSI
GPIO11 → RST
GPIO10 → DC
GPIO9  → CS
GPIO8  → BL (trực tiếp, module Ver:1.1 có transistor đệm onboard)
3V3    → VCC
GND    → GND
```

Đọc kỹ `docs/WIRING.md`; không nối tải LED nền chưa rõ dòng trực tiếp vào GPIO8.

## Thử dữ liệu từ PC

Cài Python và pyserial:

```bash
python -m pip install pyserial
python tools/serial_demo.py COM5
```

Trên macOS/Linux, thay `COM5` bằng cổng `/dev/cu.usbmodem...` hoặc `/dev/ttyACM...`. Tool tạo dữ liệu CPU/GPU/RAM/FPS giả lập. Định dạng đầy đủ ở `docs/PROTOCOL.md`.

## API web

```text
GET /api/v1/display/backlight
PUT /api/v1/display/backlight
GET /api/v1/telemetry
GET/PUT /api/v1/board/rgb
GET/POST/DELETE /api/v1/assets/boot
POST /api/v1/system/ota
GET/PUT /api/v1/ui
GET/PUT /api/v1/network
GET /api/v1/system
POST /api/v1/system/restart
GET /api/v1/cloud
POST /api/v1/cloud/check
POST /api/v1/cloud/update
WebSocket ws://192.168.4.1:81/
```

Ví dụ PUT:

```json
{"enabled":true,"brightness":65}
```

## Cấu trúc

```text
firmware/
  include/        cấu hình và interface module
  src/            display, backlight, USB, web, main
  data/           HTML/CSS/JS nạp vào LittleFS
easy-start/       sketch Arduino IDE một file
tools/            bộ phát telemetry giả lập
docs/             đấu dây và protocol
```

## Điều chỉnh panel

Nếu đỏ/xanh bị đổi, ảnh âm bản hoặc lệch, sửa `src/DisplayDevice.cpp`:

```cpp
cfg.rgb_order
cfg.invert
cfg.offset_x
cfg.offset_y
cfg.offset_rotation
```

Thông số đúng phụ thuộc module ST7789 thực tế.

## Phạm vi đã và chưa xác minh

Đã có thể kiểm tra bằng máy tính:

- Cấu trúc PlatformIO và tính nhất quán file.
- Mã firmware ở mức compiler khi toolchain tải được.
- Build profile 4 MB: RAM 74.404 byte/327.680 byte; app 1.207.009 byte/1.245.184 byte.
- Cú pháp JSON cloud và cấu trúc `.pcota`/SHA-256 ở mức máy tính.
- JavaScript và Python ở mức cú pháp.
- Bố cục web Bento và tab NTP ở mức render trình duyệt desktop.
- Đóng gói đầy đủ web assets và source.

Chỉ có thể xác minh trên phần cứng của bạn:

- Pinout thật của board clone và dung lượng flash.
- Cực tính BL của các revision TFT khác; cấu hình hiện tại dành cho module Ver:1.1 active-high.
- `invert`, RGB/BGR, rotation và offset của panel.
- SPI 40 MHz có ổn định với chiều dài dây thực tế.
- USB native có đúng với cổng USB-C của board.
- Nhiệt, nhiễu nguồn và độ bền chạy dài hạn.
- Khả năng kết nối router/hotspot cụ thể, DNS và đồng bộ NTP thực tế.
- HTTPS tới GitHub Pages/GitHub Release, đối chiếu MAC registry và OTA Internet trên bo vật lý.

Nếu ảnh nhiễu, thử giảm `TFT_SPI_HZ` xuống 20 MHz. Nếu PWM bị đảo, đổi `BL_ACTIVE_HIGH` trong `BoardConfig.h`.

## Tài liệu thư viện

- [PlatformIO Espressif 32](https://docs.platformio.org/en/stable/platforms/espressif32.html)
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- [Arduino-ESP32 LEDC](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html)
- [ArduinoJson](https://arduinojson.org/)
