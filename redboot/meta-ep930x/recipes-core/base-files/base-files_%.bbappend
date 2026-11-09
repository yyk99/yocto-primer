# Our own /etc/motd. base-files picks it from a directory named after the
# distro (files/ep930x/), as meta-poky does with files/poky/.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
