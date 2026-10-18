SUMMARY = "Minimal image for the Olimex CS-EP930x_A: boots to a shell on the serial console"

inherit core-image

IMAGE_INSTALL = "packagegroup-core-boot"

# TODO: add dropbear (ssh) once the ethernet driver is confirmed working.
