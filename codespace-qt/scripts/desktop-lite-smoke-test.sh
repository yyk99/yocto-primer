#!/usr/bin/env bash
# Quick sanity check for the desktop-lite setup (Option 3): proves the
# noVNC/Fluxbox pipeline can show a window, with zero QEMU/Yocto involved.
# Run this *inside* the desktop-lite container, in a terminal that has
# DISPLAY set (the one desktop-lite opens on the desktop automatically, or
# any shell reached via the noVNC desktop's own terminal).
set -euo pipefail

if [ -z "${DISPLAY:-}" ]; then
    echo "error: \$DISPLAY is not set - this must run inside the desktop-lite" >&2
    echo "       session (open a terminal from the noVNC desktop itself, not" >&2
    echo "       a plain Codespaces terminal)." >&2
    exit 1
fi

if ! command -v xeyes >/dev/null 2>&1; then
    echo "installing x11-apps (provides xeyes) ..."
    sudo apt-get update -qq
    sudo apt-get install -y -qq x11-apps
fi

echo "launching xeyes on \$DISPLAY=$DISPLAY - it should appear on the noVNC desktop"
exec xeyes
