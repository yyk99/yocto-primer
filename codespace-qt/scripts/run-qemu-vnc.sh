#!/usr/bin/env bash
# Boot a disk image headlessly, exporting its display over VNC instead of
# trying (and failing) to open a local SDL/GTK window.
#
# Usage: run-qemu-vnc.sh <disk-image> [vnc-display-number] [extra qemu args...]
#   vnc-display-number N -> listens on TCP port 590N (default N=1, port 5901)
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ $# -lt 1 ]; then
    echo "usage: $0 <disk-image> [vnc-display-number] [extra qemu args...]" >&2
    exit 1
fi

image="$1"; shift
vnc_display="1"
if [ $# -gt 0 ] && [[ "$1" =~ ^[0-9]+$ ]]; then
    vnc_display="$1"; shift
fi

accel_args=()
if "$here/check-kvm.sh"; then
    accel_args=(-enable-kvm)
else
    echo "falling back to software emulation (tcg) - this will be slow" >&2
fi

echo "VNC display :$vnc_display -> connect a VNC viewer to localhost:$((5900 + vnc_display)) (after forwarding that port)"

exec qemu-system-x86_64 \
    "${accel_args[@]}" \
    -m 2048 \
    -drive file="$image",if=virtio,format=raw \
    -vnc ":$vnc_display" \
    "$@"
