#!/bin/zsh
set -euo pipefail

DIR="${0:A:h}"
PYTHON=$(/usr/bin/which python3 2>/dev/null || true)
if [[ -z "$PYTHON" ]]; then
  echo "Khong tim thay Python 3. Hay cai Apple Command Line Tools."
  read "?Nhan Enter de dong."
  exit 1
fi

export PYTHONPATH="$DIR/vendor"
clear
echo "PCScreen macOS Intel Agent v1.4.0"
echo "Tac gia: Nguyen Tien Thanh - GitHub TienThanhDEV"
echo "Nhan Ctrl+C de dung."
exec "$PYTHON" "$DIR/pcscreen_mac_agent.py" "$@"
