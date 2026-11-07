SUMMARY = "Boot script that sets the clock from NTP with BusyBox ntpd"
DESCRIPTION = "The board has no battery-backed clock, so the date starts at 1970 on every boot."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://ep930x-ntpd.init"

S = "${WORKDIR}"

inherit update-rc.d

INITSCRIPT_NAME = "ntpd"
INITSCRIPT_PARAMS = "defaults 80 20"

RDEPENDS:${PN} = "busybox"

do_install() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${WORKDIR}/ep930x-ntpd.init ${D}${sysconfdir}/init.d/ntpd
}
