#!/bin/zsh
set -u
set -o pipefail

DIR="${0:A:h}"
IMAGE="$DIR/PCScreen-S3-4MB-v1.7.0-FULL.bin"
ESPTOOL="$DIR/portable_esptool/esptool.py"
LOG="$DIR/flash-macos.log"

clear
echo "======================================================"
echo " PCScreen 1.7.0 Public - ESP32-S3 Super Mini 4 MB"
echo "======================================================"

PORT=$(find /dev -maxdepth 1 \( -name 'cu.usbmodem*' -o -name 'cu.usbserial*' -o -name 'cu.wchusbserial*' \) -print | head -n 1)
if [[ -z "$PORT" ]]; then
  echo "Không tìm thấy ESP32-S3."
  echo "Giữ BOOT, nhấn RESET, thả RESET rồi thả BOOT và chạy lại."
  read "?Nhấn Enter để đóng."
  exit 1
fi

PORT_USERS=$(lsof -t "$PORT" 2>/dev/null | sort -u | tr '\n' ' ')
if [[ -n "$PORT_USERS" ]]; then
  echo "Cổng $PORT đang được tiến trình khác sử dụng:"
  lsof "$PORT" 2>/dev/null || true
  echo
  echo "Hãy đóng PCScreen Agent, Serial Monitor và Arduino IDE."
  read "STOP?Nhập STOP để script dừng các tiến trình đang giữ cổng: "
  if [[ "$STOP" == "STOP" ]]; then
    for PID in ${(z)PORT_USERS}; do kill -TERM "$PID" 2>/dev/null || true; done
    sleep 2
    PORT_USERS=$(lsof -t "$PORT" 2>/dev/null | sort -u | tr '\n' ' ')
    if [[ -n "$PORT_USERS" ]]; then
      for PID in ${(z)PORT_USERS}; do kill -KILL "$PID" 2>/dev/null || true; done
      sleep 1
    fi
  fi
  if lsof "$PORT" >/dev/null 2>&1; then
    echo "Dừng lại: cổng vẫn đang bị ứng dụng khác chiếm."
    read "?Nhấn Enter để đóng."
    exit 5
  fi
fi

PYTHON=$(command -v python3 || true)
if [[ -z "$PYTHON" || ! -f "$IMAGE" || ! -f "$ESPTOOL" ]]; then
  echo "Thiếu Python 3 hoặc tệp trong gói cài đặt."
  read "?Nhấn Enter để đóng."
  exit 1
fi

export PYTHONPATH="$DIR/portable_esptool"
echo "Cổng thiết bị: $PORT"
FLASH_INFO=$("$PYTHON" "$ESPTOOL" --chip esp32s3 --port "$PORT" --baud 115200 flash_id 2>&1)
echo "$FLASH_INFO" | tee "$LOG"
if ! echo "$FLASH_INFO" | grep -q "Detected flash size: 4MB"; then
  echo "Dừng lại: không xác nhận được flash 4 MB."
  read "?Nhấn Enter để đóng."
  exit 2
fi

echo "Bản nạp USB sẽ xóa Wi-Fi và cấu hình cũ."
read "ANSWER?Nhập YES để tiếp tục: "
if [[ "$ANSWER" != "YES" ]]; then exit 0; fi

if ! "$PYTHON" "$ESPTOOL" --chip esp32s3 --port "$PORT" --baud 115200 \
    --before default_reset --after hard_reset erase_flash 2>&1 | tee -a "$LOG"; then
  echo "Xóa flash thất bại. Kiểm tra cáp USB và đóng ứng dụng dùng cổng serial."
  read "?Nhấn Enter để đóng."
  exit 3
fi

FLASH_OK=false
for ATTEMPT in 1 2 3; do
  echo "Lần nạp $ATTEMPT/3 ở tốc độ ổn định 115200 baud..."
  if "$PYTHON" "$ESPTOOL" --chip esp32s3 --port "$PORT" --baud 115200 \
      --before default_reset --after hard_reset write_flash --flash_size 4MB \
      0x0 "$IMAGE" 2>&1 | tee -a "$LOG"; then
    FLASH_OK=true
    break
  fi
  echo "Kết nối bị gián đoạn. Đang chờ trước khi thử lại..."
  sleep 3
done

if [[ "$FLASH_OK" != "true" ]]; then
  echo "NẠP THẤT BẠI sau 3 lần thử."
  echo "Hãy đổi cáp USB dữ liệu, đổi cổng USB hoặc tháo TFT trong lúc nạp."
  echo "Log: $LOG"
  read "?Nhấn Enter để đóng."
  exit 4
fi

echo "Nạp thành công. Nhấn RESET trên bo mạch."
echo "Wi-Fi cấu hình: PCScreen-Setup / pcscreen123"
echo "Mở: http://192.168.4.1"
read "?Nhấn Enter để đóng."
