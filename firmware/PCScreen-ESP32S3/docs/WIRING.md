# Đấu dây TFT và đèn nền onboard

Tài liệu này áp dụng cho module có chữ `1.54TFT-SPI-ST7789 Ver:1.1` như ảnh người dùng cung cấp. Mặt sau module có sẵn transistor SOT-23 và mạng điện trở tại đường BL, nên BL được dùng như đầu vào điều khiển thay vì cấp tải LED trực tiếp từ ESP32.

## Pin map

| TFT ST7789 | ESP32-S3 Super Mini | Ghi chú |
|---|---:|---|
| GND | GND | Bắt buộc chung mass |
| VCC | 3V3 | Không dùng 5 V nếu chưa xác minh module |
| SCL/SCK | GPIO13 | SPI clock |
| SDA/MOSI | GPIO12 | SPI data out |
| RST | GPIO11 | Reset panel |
| DC | GPIO10 | Data/command |
| CS | GPIO9 | Chip select |
| BL | GPIO8 | PWM active-high; module có transistor đệm onboard |

```text
GPIO13 ───────── SCL/SCK
GPIO12 ───────── SDA/MOSI
GPIO11 ───────── RST
GPIO10 ───────── DC
GPIO9  ───────── CS
3V3    ───────── VCC
GND    ───────── GND
GPIO8  ───────── BL
```

Giữ dây SPI ngắn; đặt tụ 100 nF và 10 µF gần VCC/GND của TFT.

## Điều khiển BL trực tiếp

Không lắp AO3401A, 2N7002 hay transistor rời giữa GPIO8 và BL cho module Ver:1.1 này:

```text
ESP32-S3 GPIO8 ───────── TFT BL
ESP32-S3 3V3   ───────── TFT VCC
ESP32-S3 GND   ───────── TFT GND
```

Firmware dùng PWM 20 kHz, 8 bit. GPIO8 HIGH làm đèn sáng, phù hợp `BL_ACTIVE_HIGH = true`.

## Phạm vi áp dụng

Không suy rộng sơ đồ này sang mọi module ST7789. Nếu module khác hình hoặc không có chữ `Ver:1.1`:

1. Kiểm tra mặt sau có tầng transistor/điện trở BL hay không.
2. Đo dòng tại BL hoặc xem sơ đồ của nhà bán hàng.
3. Chỉ nối trực tiếp GPIO khi BL được xác nhận là đầu vào logic.
