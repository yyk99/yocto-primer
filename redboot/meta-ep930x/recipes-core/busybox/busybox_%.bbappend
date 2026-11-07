FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# poky's BusyBox config does not build ntpd; the ep930x-ntpd boot script
# uses it to set the clock (the board has no battery-backed clock).
SRC_URI += "file://ntpd.cfg"
