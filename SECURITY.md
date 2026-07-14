# Security policy

Không gửi hoặc commit GitHub PAT, Cloudflare API token, Wi-Fi password hay Worker `ADMIN_KEY`. Dùng GitHub Actions Secrets, `wrangler secret put ADMIN_KEY` và `.dev.vars` (đã được gitignore).

Nếu bí mật từng xuất hiện trong lịch sử Git/chat/log, hãy **thu hồi bí mật cũ trước**, tạo bí mật mới và xóa nó khỏi lịch sử repository. Chỉ xóa khỏi file hiện tại là chưa đủ.

Báo cáo lỗ hổng riêng cho chủ repository Nguyễn Tiến Thành (`TienThanhDEV`) thay vì đăng khóa hoặc dữ liệu thiết bị trong issue công khai.
