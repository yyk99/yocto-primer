SUMMARY = "Qt6 Widgets demo for the linuxfb platform plugin"
DESCRIPTION = "Fullscreen Qt Widgets app that shows the QPA platform and \
screen Qt picked, a live clock and a click counter. An init script starts \
it on /dev/fb0 at boot."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://CMakeLists.txt \
    file://main.cpp \
    file://linuxfb-demo.init \
"

S = "${WORKDIR}"

DEPENDS = "qtbase"

inherit qt6-cmake update-rc.d

# Build the app with -Og -g (instead of -O2) so stepping in gdb follows the
# source. The target gets the stripped binary; the symbols are split into
# linuxfb-demo-dbg, which the SDK sysroot includes (SDKIMAGE_FEATURES has
# dbg-pkgs by default) for the host's cross gdb.
DEBUG_BUILD = "1"

INITSCRIPT_NAME = "linuxfb-demo"
INITSCRIPT_PARAMS = "start 99 5 . stop 01 0 1 6 ."

do_install:append() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${WORKDIR}/linuxfb-demo.init ${D}${sysconfdir}/init.d/linuxfb-demo
}

# The app needs at least one font; Qt6 no longer ships its own.
RDEPENDS:${PN} += "ttf-bitstream-vera"
