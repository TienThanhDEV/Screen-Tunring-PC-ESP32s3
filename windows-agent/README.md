# PCScreen Windows Agent v1.5.0 — telemetry USB không chặn

Đây là ứng dụng Windows duy nhất cần chạy: LibreHardwareMonitor được tích hợp trong project, ứng dụng tự đọc cảm biến, tự tìm PCScreen qua USB CDC và gửi telemetry liên tục. Không cần mở thêm LibreHardwareMonitor hay chương trình helper riêng.

Phát triển bởi **Nguyễn Tiến Thành** — GitHub `TienThanhDEV`.

## Dữ liệu hỗ trợ

- Nhiệt độ, tải, xung nhịp và công suất CPU.
- Nhiệt độ, tải, xung nhịp, công suất, VRAM và quạt GPU AMD/NVIDIA/Intel.
- Mức sử dụng RAM và dung lượng RAM.
- Tải ổ đĩa, lưu lượng mạng, thời gian hoạt động và số tiến trình.
- Nhiệt độ hotspot GPU, nhiệt độ ổ đĩa và nhiệt độ bo mạch nếu cảm biến/driver cung cấp.
- Điều khiển bật/tắt và độ sáng màn hình PCScreen.
- Tự kết nối lại, chọn cổng COM hoặc dò tự động.
- Chọn chu kỳ gửi 250–5000 ms và tự chạy cùng Windows.
- Giao diện tự chuyển 1–4 cột theo kích thước cửa sổ và Windows DPI, không còn cắt phần bên phải.
- Thu nhỏ hoặc đóng cửa sổ xuống khay hệ thống nhưng vẫn tiếp tục gửi dữ liệu.
- Menu khay hệ thống để mở lại, tạm dừng, kết nối lại hoặc thoát hoàn toàn.
- Khởi động ẩn, lưu cổng COM/chu kỳ/độ sáng và chỉ cho chạy một phiên bản ứng dụng.
- Debounce ghi cấu hình để không ghi ổ đĩa liên tục khi kéo thanh độ sáng.
- Tự fallback sang mọi clock/load/power hợp lệ khi tên sensor khác dự kiến.
- Fallback CPU clock từ Windows Registry khi cảm biến clock mức thấp chưa có.
- Nút `Xuất báo cáo sensor` lưu toàn bộ hardware, tên sensor, loại và giá trị để map chính xác.
- Nút `Khắc phục cảm biến` mở hướng dẫn cài lớp truy cập PawnIO chính thức.
- Icon PCScreen riêng được nhúng vào file EXE, cửa sổ, taskbar và khay hệ thống; file ICO chứa 9 kích thước từ 16 đến 256 px.
- Log vận hành tự xoay vòng tại `%LOCALAPPDATA%\PCScreen\agent.log`, giới hạn khoảng 1 MB.
- Hiển thị tổng số gói telemetry đã gửi và đếm lỗi vận hành trong phiên.
- Nút mở trực tiếp `http://pcscreen.local`, thư mục log/cấu hình và hộp thông tin phiên bản.
- Menu khay hệ thống có thêm Web ESP32 và thư mục dữ liệu.

FPS không được lấy mặc định vì Windows không cung cấp chỉ số FPS chung. Có thể bổ sung PresentMon trong phiên bản sau.

## Tạo file EXE trên Windows 10/11 x64

1. Cài [.NET 8 SDK x64](https://dotnet.microsoft.com/download/dotnet/8.0).
2. Cắm ESP32-S3 bằng cáp USB dữ liệu và đóng Arduino Serial Monitor.
3. Giải nén **toàn bộ ZIP** ra một thư mục bình thường. Không chạy file build trực tiếp bên trong cửa sổ ZIP.
4. Nhấp đúp `BUILD_EXE_WINDOWS.bat`. Cửa sổ PowerShell mới sẽ luôn được giữ mở để hiển thị kết quả.
5. Sau khi hoàn tất, chạy `DIST\PCScreen-Windows-Agent.exe`.

Hoặc nhấp đúp `RUN_AGENT_WINDOWS.bat`; script sẽ tự build nếu chưa có EXE.

Ứng dụng yêu cầu quyền quản trị để LibreHardwareMonitor đọc được nhiều cảm biến hơn. Bản EXE tự build chưa có chữ ký số nên Windows SmartScreen có thể yêu cầu xác nhận lần đầu.

## Chạy nền đúng cách

- Chọn `Đóng xuống khay` để nút X chỉ ẩn cửa sổ; telemetry vẫn tiếp tục chạy.
- Nhấp đúp icon PCScreen ở khay hệ thống để mở lại.
- Chọn `Tự chạy cùng Windows` và `Khởi động ẩn` nếu muốn máy tự gửi dữ liệu sau khi đăng nhập.
- Muốn dừng hẳn, nhấp phải icon khay và chọn `Thoát hoàn toàn`.
- Mặc định 1000 ms là cân bằng. 250 ms là realtime nhưng sử dụng tài nguyên cao hơn; 2000 ms phù hợp khi ưu tiên tiết kiệm tài nguyên.

## Khi nhiệt độ, clock hoặc quạt hiện `--`

LibreHardwareMonitor 0.9.6 dùng PawnIO làm lớp truy cập phần cứng mức thấp. Quyền Administrator của EXE chưa đủ nếu PawnIO chưa được cài vào Windows.

1. Trong Agent, bấm `Khắc phục cảm biến`.
2. Tải LibreHardwareMonitor **v0.9.6 từ GitHub chính thức**.
3. Chạy LibreHardwareMonitor một lần bằng quyền Administrator và đồng ý cài PawnIO.
4. Khởi động lại Windows.
5. Đóng LibreHardwareMonitor và chạy lại PCScreen Agent. Sau bước cài một lần này, chỉ cần giữ Agent chạy.
6. Nếu GPU vẫn `--`, bấm `Xuất báo cáo sensor` và gửi file TXT được tạo.

AMD công bố AMD SMI cho GPU AMD trên Linux; MI50 trên Windows thường dùng driver không chính thức/chuyển đổi nên driver có thể không xuất telemetry qua ADL. Trường hợp báo cáo không có node `GpuAmd` hoặc node GPU không có sensor, ứng dụng không thể tự tạo nhiệt độ/quạt mà driver không cung cấp.

Nếu build không thành công, gửi lại tệp `BUILD-WINDOWS.log` nằm cạnh file build. Tệp log chứa chính xác lỗi `dotnet restore` hoặc `dotnet publish` và không chứa mật khẩu Wi-Fi.

Phiên bản 1.5.0 gửi telemetry theo cơ chế fire-and-forget thực sự: ứng dụng không chờ ACK sau mỗi mẫu nên chu kỳ 250 ms không làm nghẽn giao diện. ACK cũ được dọn trước lần ghi kế tiếp. Chỉ các lệnh điều khiển như backlight mới chờ ACK để tránh báo trạng thái sai; timeout lệnh không đóng cổng COM. Bản này bổ sung chuyển đổi toàn bộ Tiếng Việt/English, đồng thời giữ các bản sửa `NativeCommandError`, `NU1605`, responsive, system tray và chẩn đoán cảm biến; các trường JSON cũ vẫn tương thích firmware PCScreen.

## Thư viện

Project dùng `LibreHardwareMonitorLib` 0.9.6 từ NuGet. Thư viện serial `System.IO.Ports` được NuGet lấy theo đúng phiên bản phụ thuộc mà LibreHardwareMonitor yêu cầu, tránh ghim một phiên bản cũ gây lỗi downgrade. Khi build lần đầu, Windows cần Internet để tải các package này.

Mã sinh icon nằm tại `tools\generate_icon.py`; asset dùng khi build nằm trong `PCScreen.WindowsAgent\Assets`.
