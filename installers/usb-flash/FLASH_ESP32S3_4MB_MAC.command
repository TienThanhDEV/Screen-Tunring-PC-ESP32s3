#!/bin/zsh
set -u
set -o pipefail
DIR="${0:A:h}"
IMAGE="$DIR/PCScreen-S3-4MB-v1.7.0-FULL.bin"
ESPTOOL="$DIR/portable_esptool/esptool.py"
LOG="$DIR/flash-macos.log"
clear
echo "======================================================"
echo " PCScreen 1.7.0 - ESP32-S3 Super Mini 4 MB"
echo "======================================================"
PORT=$(find /dev -maxdepth 1 \( -name 'cu.usbmodem*' -o -name 'cu.usbserial*' -o -name 'cu.wchusbserial*' \) -print | head -n 1)
if [[ -z "$PORT" ]]; then
  echo "ESP32-S3 was not found. Hold BOOT, press RESET, release RESET, then release BOOT and run this installer again."
  read "?Press Enter to close."
  exit 1
fi
USERS=$(lsof -t "$PORT" 2>/dev/null | sort -u | tr '\n' ' ')
if [[ -n "$USERS" ]]; then
  echo "Close PCScreen Agent, Serial Monitor, and Arduino IDE before flashing."
  lsof "$PORT" 2>/dev/null || true
  read "STOP?Type STOP to close processes currently using this port: "
  if [[ "$STOP" == "STOP" ]]; then
    for PID in ${(z)USERS}; do kill -TERM "$PID" 2>/dev/null || true; done
    sleep 2
  fi
  if lsof "$PORT" >/dev/null 2>&1; then echo "The serial port is still busy."; read "?Press Enter to close."; exit 5; fi
fi
PYTHON=$(command -v python3 || true)
if [[ -z "$PYTHON" || ! -f "$IMAGE" || ! -f "$ESPTOOL" ]]; then echo "Python 3 or an installer file is missing."; read "?Press Enter to close."; exit 1; fi
export PYTHONPATH="$DIR/portable_esptool"
INFO=$("$PYTHON" "$ESPTOOL" --chip esp32s3 --port "$PORT" --baud 115200 flash_id 2>&1)
echo "$INFO" | tee "$LOG"
if ! echo "$INFO" | grep -q "Detected flash size: 4MB"; then echo "Could not confirm 4 MB flash; installation stopped."; read "?Press Enter to close."; exit 2; fi
echo "A full USB installation erases saved Wi-Fi and settings."
read "ANSWER?Type YES to continue: "
if [[ "$ANSWER" != "YES" ]]; then exit 0; fi
"$PYTHON" "$ESPTOOL" --chip esp32s3 --port "$PORT" --baud 115200 --before default_reset --after hard_reset erase_flash 2>&1 | tee -a "$LOG" || exit 3
OK=false
for ATTEMPT in 1 2 3; do
  echo "Flash attempt $ATTEMPT/3 at stable 115200 baud..."
  if "$PYTHON" "$ESPTOOL" --chip esp32s3 --port "$PORT" --baud 115200 --before default_reset --after hard_reset write_flash --flash_size 4MB 0x0 "$IMAGE" 2>&1 | tee -a "$LOG"; then OK=true; break; fi
  sleep 3
done
if [[ "$OK" != "true" ]]; then echo "FLASH FAILED. Try another USB data cable or USB port. Log: $LOG"; read "?Press Enter to close."; exit 4; fi
echo "Flash completed. Press RESET, then scan the Wi-Fi QR code shown on the TFT."
echo "Setup Wi-Fi: PCScreen-Setup / pcscreen123"
echo "Open http://192.168.4.1"
read "?Press Enter to close."
