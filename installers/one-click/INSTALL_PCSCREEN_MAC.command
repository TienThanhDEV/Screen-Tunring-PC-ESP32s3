#!/bin/zsh
set -u
ROOT="${0:A:h:h:h}"
clear
echo "PCScreen v1.7.0 — macOS Intel"
echo "1) Nạp firmware FULL qua USB"
echo "2) Mở PCScreen macOS Agent"
read "ANSWER?Nhập YES để nạp firmware, hoặc nhấn Enter nếu bo đã nạp: "
if [[ "$ANSWER" == "YES" ]]; then
  "$ROOT/installers/usb-flash/NAP_ESP32S3_4MB_MAC.command" || exit $?
fi
chmod +x "$ROOT/mac-agent/RUN_MAC_AGENT.command"
open "$ROOT/mac-agent/RUN_MAC_AGENT.command"
echo "Đã mở PCScreen macOS Agent. Màn hình lần đầu sẽ hiện QR cấu hình Wi-Fi."
read "?Nhấn Enter để đóng trình cài đặt."
