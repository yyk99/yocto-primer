FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# Flash support: the NOR flash chips that QEMU's virt board describes in its
# device tree, and JFFS2 on top of them.
SRC_URI += "file://mtd-jffs2.cfg"
