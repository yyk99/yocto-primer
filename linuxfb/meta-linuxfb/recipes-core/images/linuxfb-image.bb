SUMMARY = "Minimal image that runs a Qt6 app on the Linux framebuffer"
LICENSE = "MIT"

inherit core-image

IMAGE_INSTALL = " \
    packagegroup-core-boot \
    ${CORE_IMAGE_EXTRA_INSTALL} \
    linuxfb-demo \
"

IMAGE_LINGUAS = ""

# Room for poking around (and copying in other test binaries).
IMAGE_ROOTFS_EXTRA_SPACE = "131072"
