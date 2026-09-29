#!/usr/bin/env bash
# Bridge a running QEMU VNC display to a plain HTTP/WebSocket page, so it can
# be reached through the Codespaces web Ports panel (which only proxies
# HTTP) without installing a VNC viewer.
#
# Usage: novnc-bridge.sh [vnc-display-number] [web-port]
#   Requires: apt install websockify novnc
set -euo pipefail

vnc_display="${1:-1}"
web_port="${2:-6080}"
vnc_port="$((5900 + vnc_display))"

novnc_root="/usr/share/novnc"
if [ ! -d "$novnc_root" ]; then
    echo "novnc not found at $novnc_root - install it first: sudo apt install websockify novnc" >&2
    exit 1
fi

echo "serving noVNC on http://localhost:$web_port/vnc.html?autoconnect=true (forward this port in the Ports panel)"
exec websockify --web="$novnc_root" "$web_port" "localhost:$vnc_port"
