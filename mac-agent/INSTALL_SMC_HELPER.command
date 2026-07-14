#!/bin/zsh
set -euo pipefail

DIR="${0:A:h}"
SOURCE="$DIR/pcscreen_smc.c"
DESTINATION="/usr/local/libexec/pcscreen_smc"
TEMP_BINARY="/tmp/pcscreen_smc.$$.bin"
trap '/bin/rm -f "$TEMP_BINARY"' EXIT

clear
echo "======================================================"
echo " PCScreen - Cai cam bien AppleSMC cho macOS Intel"
echo "======================================================"
echo "Tac gia: Nguyen Tien Thanh - GitHub TienThanhDEV"
echo
echo "Helper chi doc AppleSMC, khong nhan tham so va se ha quyen"
echo "ngay sau khi mo ket noi. macOS se hoi mat khau quan tri mot lan."
echo

if [[ "$(/usr/bin/uname -m)" != "x86_64" ]]; then
  echo "LOI: Helper nay chi ho tro may Mac Intel x86_64."
  read "?Nhan Enter de dong."
  exit 1
fi

if [[ ! -f "$SOURCE" ]]; then
  echo "LOI: Khong tim thay $SOURCE"
  read "?Nhan Enter de dong."
  exit 1
fi

/usr/bin/xcrun --find clang >/dev/null 2>&1 || {
  echo "Thieu Apple Command Line Tools. Dang mo trinh cai dat..."
  /usr/bin/xcode-select --install || true
  echo "Cai xong Command Line Tools, hay chay lai tep nay."
  read "?Nhan Enter de dong."
  exit 1
}

/usr/bin/xcrun clang -O2 -Wall -Wextra -Werror \
  -framework IOKit -framework CoreFoundation \
  "$SOURCE" -o "$TEMP_BINARY"

sudo /usr/bin/install -d -o root -g wheel -m 0755 /usr/local/libexec
sudo /usr/bin/install -o root -g wheel -m 4755 "$TEMP_BINARY" "$DESTINATION"

echo
echo "Da cai helper: $DESTINATION"
if SENSOR_JSON=$("$DESTINATION" 2>&1); then
  echo "AppleSMC OK: $SENSOR_JSON"
else
  echo "CANH BAO: Model Mac nay khong tra ve sensor du kien: $SENSOR_JSON"
fi
read "?Nhan Enter de dong."
