#!/usr/bin/env bash
# Download a prebuilt core-image-sato (or another profile) for a qemu
# machine from downloads.yoctoproject.org, verify its checksum, and
# decompress the rootfs - so you can `runqemu` it without building anything.
#
# Usage: fetch-core-image-sato.sh <yocto-release> [machine] [profile] [outdir]
#   yocto-release  required, e.g. yocto-6.0.3 - a directory under
#                  https://downloads.yoctoproject.org/releases/yocto/
#   machine        default: qemux86-64  (any "qemu*" machine, e.g. qemuarm64)
#   profile        default: sato        (-> core-image-sato; try minimal,
#                  full-cmdline, sato-sdk, ...)
#   outdir         default: downloads/<machine>-<profile>
#
# Filenames vary between releases (some include a ".rootfs" infix, some
# don't; compression is .zst on current releases, .bz2/.gz on older ones),
# so this scrapes the actual directory listing rather than guessing.
set -euo pipefail

release="${1:?usage: $0 <yocto-release e.g. yocto-6.0.3> [machine] [profile] [outdir]}"
machine="${2:-qemux86-64}"
profile="${3:-sato}"
outdir="${4:-downloads/${machine}-${profile}}"

base="https://downloads.yoctoproject.org/releases/yocto/${release}/machines/qemu/${machine}"
image="core-image-${profile}-${machine}"

mkdir -p "$outdir"
cd "$outdir"

echo "listing $base/ ..."
listing="$(curl -fsSL "$base/")"

find_file() {
    # Prefer the un-timestamped (symlink-to-latest) name over the
    # timestamped build artifact - both match the same pattern, but the
    # timestamped one sorts first ("-2026..." < ".ext4" in ASCII).
    local matches plain
    matches="$(grep -oE 'href="[^"]*"' <<<"$listing" | sed 's/href="//;s/"$//' | grep -E "$1" || true)"
    plain="$(grep -vE '[0-9]{12,}' <<<"$matches" | sort | head -1)"
    if [ -n "$plain" ]; then
        echo "$plain"
    else
        sort <<<"$matches" | head -1
    fi
}

rootfs="$(find_file "^${image//./\\.}\.[A-Za-z0-9._-]*\.(ext4|wic)\.(zst|bz2|gz)$")"
if [ -z "$rootfs" ]; then
    echo "error: no rootfs image found for '$image' under $base/" >&2
    echo "       browse $base/ to check the release/machine/profile spelling" >&2
    exit 1
fi
kernel="bzImage-${machine}.bin"

# Derive the shared prefix (e.g. "core-image-sato-qemux86-64.rootfs") by
# stripping the compression suffix then the format suffix, so this works
# whether or not the release uses a ".rootfs" infix.
prefix="${rootfs%.*}"   # strip .zst/.bz2/.gz
prefix="${prefix%.*}"   # strip .ext4/.wic
qemuboot="${prefix}.qemuboot.conf"

fetch_and_verify() {
    local f="$1"
    if [ -f "$f" ] && [ -f "$f.sha256sum" ] && sha256sum -c "$f.sha256sum" >/dev/null 2>&1; then
        echo "-- $f (already downloaded, checksum OK) --"
        return
    fi
    echo "-- $f --"
    curl -fSL -o "$f" "$base/$f"
    curl -fSL -o "$f.sha256sum" "$base/$f.sha256sum"
    sha256sum -c "$f.sha256sum"
}

fetch_and_verify "$kernel"
fetch_and_verify "$rootfs"
fetch_and_verify "$qemuboot"

case "$rootfs" in
    *.zst)
        command -v zstd >/dev/null || { echo "installing zstd ..."; sudo apt-get update -qq && sudo apt-get install -y -qq zstd; }
        zstd -d -k -f "$rootfs"
        ;;
    *.bz2)
        bunzip2 -k -f "$rootfs"
        ;;
    *.gz)
        gunzip -k -f "$rootfs"
        ;;
    *)
        echo "warning: unrecognized compression on $rootfs, leaving it compressed" >&2
        ;;
esac

echo
echo "done - $outdir now has:"
ls -la "$kernel" "${prefix}".*

echo
echo "Next, with poky's runqemu on PATH (see ch01/README.md):"
echo "  cd $outdir && runqemu $machine core-image-$profile"
echo "or, pointing directly at the downloaded config:"
echo "  runqemu $outdir/$qemuboot"
