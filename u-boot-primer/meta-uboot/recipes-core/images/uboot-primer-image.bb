SUMMARY = "Minimal image for the U-Boot primer: boots to a shell on ttyAMA0"

inherit core-image

IMAGE_INSTALL = "packagegroup-core-boot"

# flash_erase, flashcp, ... and /proc/mtd helpers for the flash exercises
IMAGE_INSTALL:append = " mtd-utils mtd-utils-misc"
