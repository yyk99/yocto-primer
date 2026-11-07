SUMMARY = "Minimal image for the Olimex CS-EP930x_A: serial console, ssh and NTP"

inherit core-image

# dropbear; the template's debug-tweaks also allows root logins with an empty
# password over ssh (development only)
IMAGE_FEATURES += "ssh-server-dropbear"

IMAGE_INSTALL = "packagegroup-core-boot ep930x-ntpd"
