# Giao thức USB CDC cơ bản

Mỗi thông điệp là một JSON UTF-8 trên một dòng, kết thúc bằng `\n`. Giới hạn hiện tại: 512 byte/dòng. Firmware 0.6.0 trả protocol 2; chưa có CRC hoặc khung nhị phân.

## Handshake

PC gửi:

```json
{"type":"hello"}
```

ESP32 trả:

```json
{"type":"hello","device":"PCSCREEN-S3","firmware":"0.6.0","protocol":2}
```

## Telemetry

```json
{"type":"telemetry","cpuTemp":54.2,"cpuLoad":38,"cpuClockMHz":4750,"cpuPowerWatts":72,"cpuFanRpm":920,"gpuTemp":61,"gpuLoad":86,"gpuClockMHz":2520,"gpuPowerWatts":175,"gpuFanRpm":1380,"gpuMemoryLoad":46,"ramLoad":58,"memoryUsedGb":18.6,"memoryTotalGb":32,"fps":144}
```

Trường thiếu được hiển thị `--`. Sau 5 giây không có telemetry, trạng thái chuyển sang offline.

## WebSocket

Dashboard kết nối `ws://<dia-chi-esp32>:81/`. ESP32 phát một JSON telemetry mỗi giây. WebSocket chỉ đọc realtime; thay đổi setting dùng REST API để có kiểm tra giới hạn.

## Backlight

```json
{"type":"display","command":"backlight","enabled":false}
{"type":"display","command":"brightness","value":65}
```

Phản hồi thành công:

```json
{"type":"display","ok":true}
```

## Nâng cấp sản phẩm

JSON-lines đủ dễ debug cho giai đoạn đầu. Khi cần chống lỗi mạnh hơn, bọc payload bằng frame `SOF | version | type | sequence | length | payload | CRC16`; giữ version protocol để PC Agent và firmware thương lượng tương thích.
