# PCScreen Device API v2

Cloudflare Worker + Workers KV nhận heartbeat/telemetry từ ESP32 và cho Cloud Console đọc danh sách thiết bị.

## Cài đặt

1. Cài Node.js 20+, chạy `npm install` rồi `npx wrangler login`.
2. Cấu hình hiện tại để Wrangler tự cấp KV `DEVICES` khi deploy. Nếu dùng namespace đã có, thêm `id` của namespace đó vào binding `DEVICES`.
3. Thay `ADMIN_ORIGIN` bằng origin GitHub Pages của bạn.
4. Tạo khóa quản trị bằng secret, không ghi khóa vào source: `npx wrangler secret put ADMIN_KEY`.
5. Kiểm tra: `npm run check`.
6. Deploy: `npm run deploy`.
7. Dán URL `https://...workers.dev/api/v1/report` vào `telemetryEndpoint` của `docs/data/device-manifest.json`.
8. Trong Cloud Console, nhập URL gốc Worker và ADMIN_KEY để xem thiết bị online.

ESP32 tự báo cáo mỗi 5 phút. Worker giới hạn payload 8 KiB, xác minh MAC/Device ID, chỉ lưu trường đã cho phép và throttle báo cáo. `GET /api/v1/devices` bắt buộc `ADMIN_KEY` và hỗ trợ `limit`/`cursor`; `GET /api/v1/health` dùng để kiểm tra dịch vụ.

`ALLOW_ANONYMOUS_REPORTS=true` giúp thiết bị mới tự đăng ký nhưng MAC không phải cơ chế xác thực mật mã. Với hệ thống riêng tư, đặt biến này thành `false` và triển khai khóa riêng cho từng thiết bị.

Tác giả dự án: **Nguyễn Tiến Thành** — GitHub **TienThanhDEV**.
