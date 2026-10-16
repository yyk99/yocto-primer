SUMMARY = "Small console program for practising remote debugging"
DESCRIPTION = "Modes for recursion, heap data, an endless loop to attach to \
and a NULL dereference. See the comment in debug-demo.c."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://debug-demo.c"

S = "${WORKDIR}"

# -Og -g, as for linuxfb-demo; the symbols go to debug-demo-dbg.
DEBUG_BUILD = "1"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} -o debug-demo ${WORKDIR}/debug-demo.c
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 debug-demo ${D}${bindir}/debug-demo
}
