# PCScreen macOS Intel Agent v1.4.0

Agent gửi CPU, RAM, ổ đĩa, mạng, tải GPU (nếu macOS công bố), nhiệt độ và quạt AppleSMC đến ESP32-S3 qua USB CDC.

**Tác giả:** Nguyễn Tiến Thành  
**GitHub:** [TienThanhDEV](https://github.com/TienThanhDEV)

## Chạy nhanh

1. Nạp firmware v1.7.0 FULL qua USB ít nhất một lần.
2. Mở `INSTALL_SMC_HELPER.command` nếu cần nhiệt độ/quạt trên Mac Intel.
3. Mở `RUN_MAC_AGENT.command` để thử.
4. Khi chạy ổn, mở `INSTALL_AUTOSTART.command`.

Agent dùng bản pySerial đi kèm trong `vendor`, không cần `pip install`. USB được mở với DTR/RTS tắt và telemetry không chờ ACK, tránh reset ESP32 hoặc báo phản hồi chậm.

## Quyền AppleSMC

AppleSMC thường cần đặc quyền để mở. Script **biên dịch tại máy** từ `pcscreen_smc.c`, sau đó cài helper chỉ đọc vào `/usr/local/libexec/pcscreen_smc` với owner `root:wheel`. Helper:

- không nhận tham số;
- chỉ đọc danh sách key nhiệt độ/quạt cố định;
- hạ quyền vĩnh viễn ngay sau khi mở AppleSMC;
- thoát ngay sau khi xuất một dòng JSON.

Kiểm tra mã nguồn trước khi cấp quyền. Dùng `UNINSTALL.command` để gỡ cả LaunchAgent và helper.

## Giới hạn

- Chỉ helper AppleSMC dành cho Mac Intel `x86_64`; Apple Silicon không được hỗ trợ trong gói này.
- Tên/key sensor khác nhau giữa model Mac. Trường không được macOS công bố sẽ để trống.
- Chưa thể kiểm tra AppleSMC và USB thật trong CI; cần thử trên đúng máy.
