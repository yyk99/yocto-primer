SUMMARY = "Mainline Linux 6.12 LTS for Cirrus EP93xx boards"
SECTION = "kernel"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

inherit kernel

PV = "6.12.113"
SRC_URI = "https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${PV}.tar.xz \
           file://ep9301-olimex.dts \
           file://defconfig \
          "
SRC_URI[sha256sum] = "e09b6727e5c521253f0d41694900b9c38fb7604db7d35238a8197aa9f7de2428"

S = "${WORKDIR}/linux-${PV}"

COMPATIBLE_MACHINE = "ep930x"

# files/defconfig is a minimal savedefconfig: tinyconfig plus what this board
# needs (EP93xx, USB root, ethernet, flash, LEDs). Not multi_v4t_defconfig
# (31 MB uncompressed, too big for 32 MB of RAM). There is no
# KBUILD_DEFCONFIG: a plain "inherit kernel" ignores it and falls back to
# multi_v7_defconfig, which is an ARMv7 kernel. To change it:
#   bitbake -c menuconfig linux-ep93xx && bitbake -c savedefconfig linux-ep93xx
# then copy build/.../linux-ep93xx/*/build/defconfig to files/defconfig.

# Put the board's DTS into the kernel tree so the kernel's own build makes
# the .dtb (cirrus/ep9301-olimex.dtb).
do_configure:prepend() {
    cp ${WORKDIR}/ep9301-olimex.dts ${S}/arch/arm/boot/dts/cirrus/
    grep -q ep9301-olimex ${S}/arch/arm/boot/dts/cirrus/Makefile || \
        echo 'dtb-$(CONFIG_ARCH_EP93XX) += ep9301-olimex.dtb' >> ${S}/arch/arm/boot/dts/cirrus/Makefile
}

# kernel-devicetree's bundling step reads ${D}/boot/zImage, but do_install
# only writes zImage-${KERNEL_VERSION} (the plain name is made by the
# package's postinst). Link it for do_deploy and remove it afterwards, so
# the kernel-image package does not ship a file its postinst manages.
do_deploy:prepend() {
    ln -sf zImage-${KERNEL_VERSION} ${D}/${KERNEL_IMAGEDEST}/zImage
}
do_deploy:append() {
    rm -f ${D}/${KERNEL_IMAGEDEST}/zImage
}
