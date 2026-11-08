SUMMARY = "LVGL apps shown over VNC (the board has no display): a widget demo, a digital clock and a 15 puzzle"
# digits_font.c is a subset of DejaVu Sans Mono Bold (BitstreamVera license)
LICENSE = "MIT & BitstreamVera"
LIC_FILES_CHKSUM = " \
    file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302 \
    file://${COMMON_LICENSE_DIR}/BitstreamVera;md5=4157c4506f2d3cf0da1dc51015431b0c \
"

SRC_URI = " \
    file://vnc-display.c \
    file://vnc-display.h \
    file://lvgl-vnc.c \
    file://lvgl-clock.c \
    file://lvgl-15-puzzle.c \
    file://digits_font.c \
"

DEPENDS = "lvgl libvncserver"

S = "${WORKDIR}"

do_compile() {
    # lvgl installs lv_conf.h in include/lvgl/, which lvgl.h finds only from there
    for app in lvgl-vnc lvgl-clock lvgl-15-puzzle; do
        extra=""
        [ $app = lvgl-clock ] && extra=digits_font.c
        ${CC} ${CFLAGS} -I${STAGING_INCDIR}/lvgl ${LDFLAGS} $app.c $extra vnc-display.c \
            -o $app -llvgl -lvncserver -lm
    done
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 lvgl-vnc lvgl-clock lvgl-15-puzzle ${D}${bindir}/
}
