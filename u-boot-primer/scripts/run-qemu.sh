#!/usr/bin/env bash
# Boot U-Boot from emulated NOR flash in QEMU, with the uboot-primer-image
# disk attached.
#
# Usage: scripts/run-qemu.sh [extra qemu args...]
#   BUILDDIR   build directory (default: <u-boot-primer>/build)
#   QEMU       qemu-system-aarch64 to use (default: host one, else the one
#              `bitbake qemu-system-native` built under BUILDDIR)
#   PERSIST=1  write changes to the disk image (default: discarded on exit)
#   RESET_ENV=1  erase the saved U-Boot environment (flash1.img)
#
# Flash layout (QEMU 'virt' has two 64 MiB CFI NOR banks):
#   flash0  0x00000000  u-boot.bin       (rebuilt from the deploy dir every run)
#           0x00400000  kernel Image     (if built)
#   flash1  0x04000000  U-Boot environment; `saveenv` persists in flash1.img
#
# Quit QEMU with Ctrl-a x.
set -euo pipefail

machine=qemuarm64-uboot
top=$(cd "$(dirname "$0")/.." && pwd)
build=${BUILDDIR:-$top/build}
deploy=$build/tmp/deploy/images/$machine
flashdir=$build/flash

flash_size=$((64 * 1024 * 1024))
kernel_offset=$((4 * 1024 * 1024))

qemu=${QEMU:-$(command -v qemu-system-aarch64 || true)}
if [ -z "$qemu" ]; then
    qemu=$(find "$build/tmp/sysroots-components" -name qemu-system-aarch64 \
           -type f 2>/dev/null | head -n1)
fi
[ -n "$qemu" ] || { echo "no qemu-system-aarch64: apt install qemu-system-arm, or bitbake qemu-system-native" >&2; exit 1; }

uboot=$deploy/u-boot.bin
[ -f "$uboot" ] || { echo "missing $uboot: run 'bitbake u-boot'" >&2; exit 1; }
kernel=$deploy/Image
disk=$(ls "$deploy"/uboot-primer-image-"$machine"*.wic 2>/dev/null | head -n1 || true)

# A flash image is the whole bank, so pad with 0xff (the erased state).
# Writing less than the full bank would make QEMU refuse the file.
pad_to_flash() {
    local size
    size=$(stat -L -c %s "$1")
    [ "$size" -le "$flash_size" ] || { echo "$1 does not fit in flash" >&2; exit 1; }
    head -c $((flash_size - size)) /dev/zero | tr '\0' '\377' >> "$1"
}

mkdir -p "$flashdir"

flash0=$flashdir/flash0.img
cp "$uboot" "$flash0"
if [ -f "$kernel" ]; then
    ksize=$(stat -L -c %s "$kernel")
    [ $((kernel_offset + ksize)) -le "$flash_size" ] || { echo "kernel does not fit in flash0" >&2; exit 1; }
    # u-boot.bin, 0xff fill up to the kernel offset, then the kernel.
    { cat "$uboot"
      head -c $((kernel_offset - $(stat -L -c %s "$uboot"))) /dev/zero | tr '\0' '\377'
      cat "$kernel"; } > "$flash0"
    echo "kernel: $kernel ($ksize bytes) at flash0 0x$(printf %x "$kernel_offset")"
else
    echo "kernel: not built, flash0 holds u-boot only"
fi
pad_to_flash "$flash0"

flash1=$flashdir/flash1.img
if [ ! -f "$flash1" ] || [ "${RESET_ENV:-0}" = 1 ]; then
    : > "$flash1"
    pad_to_flash "$flash1"
fi

disk_args=()
if [ -n "$disk" ]; then
    # snapshot=on discards disk writes on exit, unless PERSIST=1. It is set
    # per drive so it does not also apply to the flash images.
    snap=on
    [ "${PERSIST:-0}" = 1 ] && snap=off
    disk_args=(-drive if=none,id=disk0,file="$disk",format=raw,snapshot=$snap
               -device virtio-blk-pci,drive=disk0)
    echo "disk:   $disk"
else
    echo "disk:   none (run 'bitbake uboot-primer-image' for the rootfs)"
fi

echo "qemu:   $qemu"
echo "flash:  $flashdir"
exec "$qemu" -machine virt -cpu cortex-a57 -smp 2 -m 1G -nographic \
    -drive if=pflash,unit=0,format=raw,file="$flash0",snapshot=off \
    -drive if=pflash,unit=1,format=raw,file="$flash1",snapshot=off \
    "${disk_args[@]}" \
    -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
    "$@"
