SUMMARY = "Minimal image for the U-Boot primer: boots to a shell on ttyAMA0"

inherit core-image

IMAGE_INSTALL = "packagegroup-core-boot"

# wic copies IMAGE_BOOT_FILES (the kernel Image) out of the deploy dir, so the
# kernel has to be deployed first.
do_image_wic[depends] += "virtual/kernel:do_deploy"
