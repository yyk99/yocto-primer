SUMMARY = "LVGL demo shown over VNC (the board has no display)"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://lvgl-vnc.c"

DEPENDS = "lvgl libvncserver"

S = "${WORKDIR}"

do_compile() {
    # lvgl installs lv_conf.h in include/lvgl/, which lvgl.h finds only from there
    ${CC} ${CFLAGS} -I${STAGING_INCDIR}/lvgl ${LDFLAGS} lvgl-vnc.c -o lvgl-vnc -llvgl -lvncserver -lm
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 lvgl-vnc ${D}${bindir}/lvgl-vnc
}
