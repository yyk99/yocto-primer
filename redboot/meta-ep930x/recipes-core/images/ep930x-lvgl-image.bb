SUMMARY = "LVGL library and the VNC demo as a tar.gz file system (not bootable)"
LICENSE = "MIT"

# Not a root file system: only the demo and what it links against (LVGL,
# LibVNCServer, musl). Unpack it into a directory on the board's root, see
# README.md.
inherit image

IMAGE_FSTYPES = "tar.gz"
IMAGE_FEATURES = ""
IMAGE_LINGUAS = ""
NO_RECOMMENDATIONS = "1"
IMAGE_INSTALL = "ep930x-lvgl-demo"
