#!/usr/bin/env bash
# Boot a kernel+rootfs pair fetched by fetch-core-image-sato.sh directly
# with qemu-system-*, without needing poky's runqemu on PATH. Reads the
# downloaded .qemuboot.conf to build the command line (cpu, memory, kernel
# cmdline, ...) instead of hardcoding it for one profile/machine - the
# same file drives whatever machine/profile you actually fetched.
#
# Usage: run-core-image.sh [dir-or-qemuboot.conf] [extra qemu args...]
#   dir-or-qemuboot.conf  default: .   A directory containing exactly one
#                         *.qemuboot.conf (as fetch-core-image-sato.sh
#                         produces), or a path to one directly.
#   extra qemu args       passed straight through to qemu-system-* - e.g.
#                         `-vnc :1 -display none` to reuse the Option 1/2
#                         VNC setup instead of opening a local window.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

target="${1:-.}"
[ $# -gt 0 ] && shift

if [ ! -e "$target" ]; then
    echo "error: '$target' does not exist - did you run scripts/fetch-core-image-sato.sh first?" >&2
    exit 1
fi

if [ -d "$target" ]; then
    mapfile -t candidates < <(find "$target" -maxdepth 1 -name '*.qemuboot.conf')
    case "${#candidates[@]}" in
        0) echo "error: no *.qemuboot.conf found in '$target'" >&2; exit 1 ;;
        1) qemuboot="${candidates[0]}" ;;
        *) echo "error: multiple *.qemuboot.conf in '$target' - pass one directly:" >&2
           printf '  %s\n' "${candidates[@]}" >&2
           exit 1 ;;
    esac
else
    qemuboot="$target"
fi
dir="$(cd "$(dirname "$qemuboot")" && pwd)"

# "key = value" lines: the literal " = " (space-equals-space) is always the
# separator, so splitting on it leaves internal, space-free "="s in the
# value (e.g. "i8042=off", "mac=@MAC@") intact.
conf_val() {
    grep -E "^$1 = " "$qemuboot" | sed -E "s/^$1 = //" | tail -1
}

machine="$(conf_val machine)"
kernel_imagetype="$(conf_val kernel_imagetype)"
image_link_name="$(conf_val image_link_name)"
fstype="$(conf_val qb_default_fstype)"
fstype="${fstype%.zst}"; fstype="${fstype%.bz2}"; fstype="${fstype%.gz}"
system_name="$(conf_val qb_system_name)"
mem="$(conf_val qb_mem)"
smp="$(conf_val qb_smp)"
serial_opt="$(conf_val qb_serial_opt)"
opt_append="$(conf_val qb_opt_append)"
rng="$(conf_val qb_rng)"
network_device="$(conf_val qb_network_device)"
cmdline_ip="$(conf_val qb_cmdline_ip_slirp)"
cmdline_append="$(conf_val qb_kernel_cmdline_append)"

kernel="$dir/${kernel_imagetype}-${machine}.bin"
rootfs="$dir/${image_link_name}.${fstype}"

# Decompress on demand: if only the compressed rootfs is present (e.g. the
# large decompressed copy was deleted to save disk space), regenerate it
# and keep the compressed original either way.
if [ ! -f "$rootfs" ]; then
    for ext in zst bz2 gz; do
        compressed="$rootfs.$ext"
        [ -f "$compressed" ] || continue
        echo "$rootfs missing, decompressing $compressed ..." >&2
        case "$ext" in
            zst) zstd -d -k -f "$compressed" ;;
            bz2) bunzip2 -k -f "$compressed" ;;
            gz)  gunzip -k -f "$compressed" ;;
        esac
        break
    done
fi

[ -f "$kernel" ] || { echo "error: expected file not found: $kernel" >&2; exit 1; }
[ -f "$rootfs" ] || { echo "error: expected file not found: $rootfs (no compressed .zst/.bz2/.gz found to decompress either)" >&2; exit 1; }

if "$here/check-kvm.sh" >/dev/null 2>&1; then
    cpu="$(conf_val qb_cpu_kvm)"
    kvm_flag="-enable-kvm"
else
    echo "no KVM - falling back to software emulation (slower)" >&2
    cpu="$(conf_val qb_cpu)"
    kvm_flag=""
fi

mac="52:54:00:12:34:56"
network_device="${network_device//@MAC@/$mac}"

echo "booting $rootfs on $machine ($system_name)"
# Intentionally unquoted below: each of these variables holds several
# space-separated qemu flags (e.g. "-cpu Skylake-Client -machine q35,...")
# that must be split into separate argv entries.
exec "$system_name" \
    $kvm_flag $cpu $mem $smp \
    -netdev user,id=net0 $network_device \
    -drive file="$rootfs",if=virtio,format=raw \
    -kernel "$kernel" \
    -append "root=/dev/vda rw $cmdline_ip $cmdline_append" \
    $serial_opt $opt_append $rng \
    "$@"
