SUMMARY = "Minimal image that runs a Qt6 app on the Linux framebuffer"
LICENSE = "MIT"

inherit core-image

IMAGE_INSTALL = " \
    packagegroup-core-boot \
    ${CORE_IMAGE_EXTRA_INSTALL} \
    linuxfb-demo \
    gdbserver \
    debug-demo \
"

# SSH (dropbear) to log in and copy files; gdbserver above is the debug
# server for remote sessions from the host.
IMAGE_FEATURES += "ssh-server-dropbear"

IMAGE_LINGUAS = ""

# Room for poking around (and copying in other test binaries).
IMAGE_ROOTFS_EXTRA_SPACE = "131072"
