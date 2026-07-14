#!/bin/zsh
set -euo pipefail

DIR="${0:A:h}"
PYTHON=$(/usr/bin/which python3 2>/dev/null || true)
PLIST="$HOME/Library/LaunchAgents/vn.pcscreen.mac-agent.plist"
LABEL="vn.pcscreen.mac-agent"

if [[ -z "$PYTHON" ]]; then
  echo "Khong tim thay Python 3. Hay cai Apple Command Line Tools."
  read "?Nhan Enter de dong."
  exit 1
fi

if [[ ! -x /usr/local/libexec/pcscreen_smc ]]; then
  echo "Chua co SMC helper. Nhiet do/quat co the trong."
  echo "Nen chay INSTALL_SMC_HELPER.command truoc."
fi

/bin/mkdir -p "$HOME/Library/LaunchAgents" "$HOME/Library/Logs"
/bin/launchctl bootout "gui/$(/usr/bin/id -u)/$LABEL" >/dev/null 2>&1 || true
/usr/bin/python3 - "$PLIST" "$PYTHON" "$DIR" <<'PY'
import pathlib
import plistlib
import sys

path, python, folder = sys.argv[1:]
home = pathlib.Path.home()
data = {
    "Label": "vn.pcscreen.mac-agent",
    "ProgramArguments": [python, folder + "/pcscreen_mac_agent.py"],
    "EnvironmentVariables": {
        "PYTHONPATH": folder + "/vendor",
        "PCSCREEN_SMC_HELPER": "/usr/local/libexec/pcscreen_smc",
    },
    "RunAtLoad": True,
    "KeepAlive": {"SuccessfulExit": False},
    "ThrottleInterval": 5,
    "ProcessType": "Background",
    "StandardOutPath": str(home / "Library/Logs/PCScreen-Agent.log"),
    "StandardErrorPath": str(home / "Library/Logs/PCScreen-Agent-error.log"),
}
with open(path, "wb") as stream:
    plistlib.dump(data, stream)
PY

/bin/launchctl bootstrap "gui/$(/usr/bin/id -u)" "$PLIST"
echo "Da cai PCScreen Agent tu khoi dong."
echo "Log: $HOME/Library/Logs/PCScreen-Agent.log"
read "?Nhan Enter de dong."
