SUMMARY = "Mainline Linux 6.12 LTS for Cirrus EP93xx boards"
SECTION = "kernel"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

inherit kernel

PV = "6.12.113"
SRC_URI = "https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${PV}.tar.xz \
           file://ep9301-olimex.dts \
           file://ep930x.cfg \
          "
SRC_URI[sha256sum] = "e09b6727e5c521253f0d41694900b9c38fb7604db7d35238a8197aa9f7de2428"

S = "${WORKDIR}/linux-${PV}"

COMPATIBLE_MACHINE = "ep930x"

# Mainline's multi_v4t_defconfig covers ARMv4T SoCs including ep93xx;
# ep930x.cfg is merged on top (appended DTB, USB storage, rootfs drivers).
KBUILD_DEFCONFIG = "multi_v4t_defconfig"

# Put the board's DTS into the kernel tree so the kernel's own build makes
# the .dtb (cirrus/ep9301-olimex.dtb).
do_configure:prepend() {
    cp ${WORKDIR}/ep9301-olimex.dts ${S}/arch/arm/boot/dts/cirrus/
    grep -q ep9301-olimex ${S}/arch/arm/boot/dts/cirrus/Makefile || \
        echo 'dtb-$(CONFIG_ARCH_EP93XX) += ep9301-olimex.dtb' >> ${S}/arch/arm/boot/dts/cirrus/Makefile
}

# kernel.bbclass has already run the defconfig by now: merge the fragment
# and let the kernel resolve the dependencies.
do_configure:append() {
    cat ${WORKDIR}/ep930x.cfg >> ${B}/.config
    oe_runmake -C ${S} O=${B} olddefconfig
}
