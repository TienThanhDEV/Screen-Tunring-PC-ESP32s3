# PCScreen Cloud Admin — GitHub Pages

Website tĩnh dành cho quản trị viên quản lý hiệu ứng RGB, manifest firmware,
registry thiết bị và tài liệu cho ESP32-S3 Super Mini 4 MB.

## Website làm được gì

- Giao diện quản trị responsive, dark/light.
- Xem và chỉnh `effects.json`, preview màu và tốc độ.
- Tạo `firmware-manifest.json`, kiểm tra phiên bản, URL HTTPS, size, SHA-256.
- Registry thiết bị và danh mục tài liệu.
- Chế độ demo: sửa rồi tải JSON về máy.
- Chế độ admin: dùng GitHub Contents API để commit JSON thẳng vào repo.
- Token chỉ nằm trong RAM của tab và bị xóa khi tải lại/đóng tab.
- ESP32 đọc dữ liệu qua GitHub Pages bằng HTTPS, không cần token.

## Giới hạn bảo mật cần hiểu

GitHub Pages là website tĩnh công khai. HTML/CSS/JSON và dữ liệu đọc **không thể
giữ riêng cho admin**. Bản này bảo vệ thao tác ghi bằng fine-grained GitHub PAT;
người không có token chỉ xem/tải dữ liệu công khai. Không ghi token vào file,
JavaScript, GitHub Secret, URL hoặc firmware.

Nếu cần tài khoản admin thực sự, nhiều người dùng, audit log hoặc telemetry
ESP32 thời gian thực, cần backend có OAuth/API. GitHub Pages một mình không làm
được phần đó.

## Cấu trúc gói

```text
PCScreen-Cloud-Admin/
├── docs/                         # tải nguyên thư mục này lên repo
│   ├── index.html
│   ├── styles.css
│   ├── app.js
│   ├── .nojekyll
│   ├── assets/favicon.svg
│   └── data/
│       ├── device-manifest.json
│       ├── effects.json
│       ├── firmware-manifest.json
│       ├── devices.json
│       └── docs.json
├── firmware-example/             # mã ESP32 đọc dữ liệu cloud
├── app/                          # wrapper để kiểm tra build giao diện
└── README.md
```

## Cách 1 — Tải lên bằng Terminal trên macOS Intel

Yêu cầu đã cài Git. Mở Terminal:

```bash
cd ~/Downloads
git clone https://github.com/TienThanhDEV/Screen-Tunring-PC-ESP32s3.git
cd Screen-Tunring-PC-ESP32s3
```

Chép thư mục `docs` và `firmware-example` của gói này vào repository, sau đó:

```bash
git add docs firmware-example
git commit -m "Add PCScreen Cloud Admin"
git push origin main
```

Nếu GitHub hỏi đăng nhập, dùng GitHub Desktop hoặc PAT; mật khẩu tài khoản GitHub
không dùng thay cho token trên Git HTTPS.

Trong gói cũng có `UPLOAD-TO-GITHUB-MAC.command`: nhấp đúp, nhập đường dẫn repo
đã clone, xem danh sách thay đổi rồi gõ `yes` để commit/push. Script luôn hỏi xác
nhận và không tự lưu token.

## Cách 2 — Tải lên bằng website GitHub

1. Mở repo `TienThanhDEV/Screen-Tunring-PC-ESP32s3`.
2. Chọn **Add file → Upload files**.
3. Kéo thư mục `docs` vào, xác nhận cấu trúc là `docs/index.html` và
   `docs/data/*.json`.
4. Commit vào nhánh `main`.

Terminal/GitHub Desktop đáng tin cậy hơn khi upload cả cây thư mục.

## Bật GitHub Pages

1. Trong repo chọn **Settings → Pages**.
2. Ở **Build and deployment**, Source chọn **Deploy from a branch**.
3. Branch chọn `main`, folder chọn `/docs`, nhấn **Save**.
4. Chờ workflow Pages hoàn tất rồi mở:

```text
https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/
```

Tài liệu chính thức: [Configuring a publishing source](https://docs.github.com/en/pages/getting-started-with-github-pages/configuring-a-publishing-source-for-your-github-pages-site).

## Tạo token quản trị tối thiểu

1. GitHub avatar → **Settings → Developer settings**.
2. **Personal access tokens → Fine-grained tokens → Generate new token**.
3. Resource owner: tài khoản `TienThanhDEV`.
4. Repository access: **Only select repositories** →
   `Screen-Tunring-PC-ESP32s3`.
5. Repository permissions: **Contents → Read and write**; Metadata để mặc định
   read-only.
6. Đặt ngày hết hạn ngắn, tạo token và sao chép một lần.
7. Mở Cloud Console, dán token, nhấn **Kết nối quản trị**.

Token không được lưu lại. Refresh trang sẽ phải nhập lại. Có thể thu hồi token
ngay khi quản trị xong. Tài liệu: [Managing personal access tokens](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens) và [Repository Contents API](https://docs.github.com/en/rest/repos/contents).

## Đưa firmware `.pcota` lên GitHub Release

Không đặt firmware lớn trong thư mục Pages. Trong repo:

1. Mở **Releases → Draft a new release**.
2. Tag `v0.6.0`, tiêu đề `PCScreen v0.6.0`.
3. Upload `PCScreen-S3-4MB-v0.6.0.pcota`.
4. Publish release.
5. Sao chép URL tải file vào tab **Firmware**.
6. Tính SHA-256 trên macOS:

```bash
shasum -a 256 PCScreen-S3-4MB-v0.6.0.pcota
stat -f %z PCScreen-S3-4MB-v0.6.0.pcota
```

7. Nhập hash 64 ký tự và kích thước byte, lưu manifest, thử trên một board mẫu.

Tài liệu: [Managing releases](https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository).

## Kết nối ESP32-S3

URL dữ liệu mặc định:

```text
https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/data
```

Chép `firmware-example/CloudDataClient.h` và `.cpp` vào firmware chính, hoặc mở
`GitHubCloudExample.ino`. Cần Arduino-ESP32 3.x và ArduinoJson 7.x.

Nguyên tắc:

- ESP32 chỉ gọi `GET`, tuyệt đối không có GitHub token.
- Poll tối thiểu 15 phút và giữ bản cấu hình hợp lệ gần nhất.
- Chỉ áp dụng schema hỗ trợ.
- OTA phải kiểm tra board `esp32-s3-super-mini`, flash 4 MB, size và SHA-256.
- Mã mẫu dùng `setInsecure()` cho thử nghiệm. Bản sản phẩm cần CA certificate.

Chi tiết: [docs/ESP32-CLOUD-INTEGRATION.md](docs/ESP32-CLOUD-INTEGRATION.md).

## Cập nhật dữ liệu

1. Mở Cloud Console và kết nối token.
2. Chỉnh hiệu ứng hoặc firmware.
3. Nhấn **Xuất bản thay đổi**.
4. Xem danh sách file và commit lên GitHub.
5. Chờ Pages build. ESP32 nhận dữ liệu ở chu kỳ kế tiếp.

Nếu không muốn nhập token trong trình duyệt, dùng **Xem bản demo**, chỉnh dữ
liệu, tải JSON rồi tự chép vào `docs/data` và `git push`.

## Kiểm tra cục bộ

Không mở trực tiếp `index.html` bằng `file://` vì trình duyệt có thể chặn fetch
JSON. Chạy HTTP server:

```bash
cd docs
python3 -m http.server 8080
```

Mở `http://localhost:8080`, chọn **Xem bản demo**.

Trên macOS có thể nhấp đúp `PREVIEW-MAC.command` để làm hai bước này tự động.

## Những phần chưa thể xác minh bằng phần cứng

- Không có quyền push vào repo của người dùng nên gói chưa được xuất bản thật.
- Chưa thử HTTPS/JSON trên ESP32-S3 vật lý trong môi trường này.
- Chưa thực hiện OTA cloud trên board; bắt buộc thử với một thiết bị trước khi
  bật `mandatory`.
- GitHub Pages không nhận telemetry, heartbeat hay log từ thiết bị.
