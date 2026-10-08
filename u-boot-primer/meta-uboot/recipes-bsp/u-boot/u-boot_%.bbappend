FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# Config fragment on top of qemu_arm64_defconfig. Edit files/primer.cfg to
# practice changing the U-Boot configuration.
SRC_URI += "file://primer.cfg"
