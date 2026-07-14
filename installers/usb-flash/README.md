# Nạp USB PCScreen v1.7.0

Đây là cách cài **bắt buộc một lần** khi chuyển từ v1.5.x trở xuống vì bảng phân vùng 4 MB đã thay đổi từ v1.6.0.

## macOS Intel

1. Đóng PCScreen Agent, Serial Monitor và Arduino IDE.
2. Cắm cáp USB dữ liệu; nếu cần vào bootloader: giữ BOOT, nhấn/thả RESET rồi thả BOOT.
3. Nhấp phải `NAP_ESP32S3_4MB_MAC.command` → **Open**.
4. Script chỉ tiếp tục khi xác nhận flash đúng 4 MB và yêu cầu nhập `YES` trước khi xóa.

## Windows

1. Cài Python 3 nếu máy chưa có.
2. Đóng mọi ứng dụng giữ cổng COM.
3. Chạy `NAP_ESP32S3_4MB_WINDOWS.bat`, nhập cổng như `COM5`, sau đó nhập `YES`.

Hai script đều nạp ở 115200 baud để ổn định và dùng `PCScreen-S3-4MB-v1.7.0-FULL.bin` tại offset `0x0`. Sau khi hoàn tất: Wi-Fi `PCScreen-Setup`, mật khẩu `pcscreen123`, địa chỉ `http://192.168.4.1`.

`PCScreen-S3-4MB-v1.7.0.pcota` dùng cho thiết bị đã nạp FULL v1.6.0 hoặc mới hơn và đang dùng đúng bảng phân vùng hiện tại.

Tác giả: **Nguyễn Tiến Thành** — GitHub **TienThanhDEV**.
