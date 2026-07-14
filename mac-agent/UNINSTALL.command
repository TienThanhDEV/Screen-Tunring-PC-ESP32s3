#!/bin/zsh
set -euo pipefail

PLIST="$HOME/Library/LaunchAgents/vn.pcscreen.mac-agent.plist"
/bin/launchctl bootout "gui/$(/usr/bin/id -u)/vn.pcscreen.mac-agent" \
  >/dev/null 2>&1 || true
/bin/rm -f "$PLIST"

if [[ -e /usr/local/libexec/pcscreen_smc ]]; then
  echo "Go helper AppleSMC can mat khau quan tri."
  sudo /bin/rm -f /usr/local/libexec/pcscreen_smc
fi

echo "Da go PCScreen macOS Agent va helper AppleSMC."
read "?Nhan Enter de dong."
