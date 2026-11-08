# Olimex CS-EP930x_A: modern Linux with Yocto

Scaffold for building a current mainline Linux (6.12 LTS) and a musl root
filesystem for the Olimex CS-EP930x_A board (Cirrus EP9301/EP9302, ARM920T,
RedBoot). **Status: scaffold only. It parses; it has not been built or
booted.** The board's stock instructions are in
`CS-EP930x_A/EP930x quickstart guide.txt`.

## Layout

```
redboot/
├── setup-layers, setup-layers.json     pinned poky (scarthgap), same as u-boot-primer
├── CS-EP930x_A/                        vendor quickstart guide
└── meta-ep930x/
    ├── conf/machine/ep930x.conf        armv4t, zImage with appended DTB
    ├── conf/templates/default/         TEMPLATECONF: local.conf + bblayers.conf
    ├── recipes-kernel/linux/           linux-ep93xx_6.12.bb, ep930x.cfg, ep9301-olimex.dts
    └── recipes-core/images/ep930x-image.bb
```

## Hardware (from the board's DVD)

`CS-E9302-SCH.gif` is the schematic (the DVD ships the same one for the
EP9301 board; the chip on it is labelled EP9301). It shows: 32 MB SDRAM
(K4S561632C), 16 MB NOR flash (JS28F128J3C120, x16, CSN6 = `0x60000000`),
KS8721BL ethernet PHY, two RS232 ports, two USB host ports, SD card slot,
IrDA, AT25F1024 SPI flash, green and red LEDs. The DVD also has RedBoot
binaries, 2.4/2.6 images, the Cirrus kernel patches, `rootdir.tgz` and the
EP9301 user guide.

## What RedBoot reports (read from the board)

```
RedBoot v2_0, built Aug 22 2006, "Cirrus Logic EDB9302 Board (ARM920T) Rev A"
RAM:   0x00000000-0x02000000 (32 MB, contiguous)
FLASH: 0x60000000-0x61000000 (16 MB, 128 blocks of 0x20000)

Name              FLASH addr  Mem addr    Length      Entry point
RedBoot           0x60000000  0x60000000  0x00040000  0x00000000
RedBoot config    0x60FC0000  0x60FC0000  0x00001000  0x00000000
FIS directory     0x60FE0000  0x60FE0000  0x00020000  0x00000000
netbsd            0x60040000  0x00200000  0x00500000  0x00200000
netbsd_install    0x60540000  0x00200000  0x00500000  0x00200000
ramdisk.gz        0x60A40000  0x00800000  0x00300000  0x00800000
zImage            0x60D40000  0x00080000  0x00180000  0x00080000
```

Stock boot script (`fconfig -l`), timeout 5 s, no BOOTP:

```
fis load ramdisk.gz
fis load zImage
exec -r 0x800000 -s 0x800000 -c "console=ttyAM root=/dev/ram"
```

Network: board `192.168.0.93/24`, no gateway, default server
`192.168.0.20`. The MAC was `00:00:00:00:54:33`, a placeholder with no vendor
prefix; it is now `02:00:00:00:54:33` (locally administered), set with
`fconfig` (`Network hardware address [MAC]`, typed as `0x02:0x00:0x00:0x00:0x54:0x33`).
The 6.12 `ep93xx_eth` driver does not choose an address: it reads back what
RedBoot wrote into the MAC registers at start-up, and uses a random one only if
they are all zero. The 6.12 `zImage` bundle (2.4 MB) does not fit the stock
1.5 MB `zImage` slot, so it lives in its own 3 MB `kernel` slot, see the flash
layout in `TODO.txt`.

## Design

- **Appended DTB.** Mainline ep93xx is devicetree-only, but RedBoot passes
  only ATAGS. The DTB is appended to `zImage` (`KERNEL_DEVICETREE_BUNDLE`),
  and the kernel takes memory and the command line from the ATAGS.
- **musl, soft-float.** ARMv4T, no FPU.
- **Root on the SD card.** The slot is MMC over SPI (`spi0`, chip select
  `gpio5 3`, wiring from the stock boot log); `root=/dev/mmcblk0p1 rootwait`.
  USB root is unreliable, see Open items.

## Kernel config

`files/defconfig` is a minimal config (`tinyconfig` plus the board's
drivers, saved with `savedefconfig`): the `zImage` with the DTB appended is
2.4 MB. A plain `inherit kernel` ignores `KBUILD_DEFCONFIG` and falls back to
`multi_v7_defconfig`, which gave a 31 MB ARMv7 kernel that cannot run on this
CPU, so the recipe ships its own `defconfig`.

## Open items

- `ep9301-olimex.dts` is derived from mainline's EDB9302 board. The PHY
  address (KS8721BL, strapping not legible in the GIF) is still a guess.
- It is unverified that musl and every package build for ARMv4T.
- **USB mass storage is flaky.** The OHCI host enumerates a stick or card
  reader, but sustained reads fail at random (`cc 8` data overrun, then
  resets) on two devices and both sockets, at every request size. The stock
  2.6.24 kernel reads 10 MB fine; a long stock read has not been compared.
- The SD card-detect and write-protect lines are not wired up (`broken-cd`);
  the chip select `gpio5 3` is inferred, not read from the schematic.
- RedBoot's ATAGS describe RAM as four 8 MB banks (0x0, 0x1000000, 0x4000000,
  0x5000000), not one contiguous 32 MB.
- The clock has no battery: `date` starts at 1970 on every boot. The image
  runs BusyBox `ntpd` against `pool.ntp.org` (`ep930x-ntpd`); it needs DHCP
  and DNS, and takes a short while after boot to set the time.
- ssh (dropbear) is in the image, and `debug-tweaks` allows root logins with
  an empty password. Development only: remove `debug-tweaks` from
  `EXTRA_IMAGE_FEATURES` before any real use.
- `tinyconfig` turns off options that userland expects; more may surface.
  `CONFIG_COMPAT_32BIT_TIME=y` is needed because musl calls the old 32-bit
  time syscalls (`utimensat`, `clock_settime`) while the date fits in 32 bits;
  without it `touch`, `hwclock` and `udevd` fail with `ENOSYS`.

## Build

```
cd redboot
./setup-layers --destdir .
TEMPLATECONF=$PWD/meta-ep930x/conf/templates/default source poky/oe-init-build-env build
bitbake ep930x-image
```

## Boot the kernel over TFTP

Verified on the board: the kernel boots to `printk: legacy console [ttyAM0]
enabled`. uart0 carries the PL010 peripheral id, so the console is `ttyAM0`,
not `ttyAMA0`.

On the PC (192.168.68.57), serve the deploy directory (leave it running):

```
sudo dnsmasq --no-daemon --port=0 --enable-tftp --tftp-root=$PWD/build/tmp/deploy/images/ep930x --listen-address=192.168.68.57 --bind-interfaces
```

At the `RedBoot>` prompt (give the board a free address on the same /24):

```
ip_address -l 192.168.68.100 -h 192.168.68.57
load -v -r -b 0x00080000 zImage-ep9301-olimex.dtb.bin
exec -r 0x800000 -s 0x800000 -c "console=ttyAM0,57600"
```

Keep the full `exec -r ... -s ...` form: a bare `exec -c ... <addr>` printed
nothing even with the stock kernel. Add `earlyprintk` to the command line to
see messages before the console registers (needs `CONFIG_DEBUG_LL`).

## Boot with the root on the SD card

Write the image to the card on the PC, from the `redboot/` directory. This
erases the card. Check which device it is first:

```
lsblk -o NAME,SIZE,FSTYPE,LABEL,MOUNTPOINT,TRAN,MODEL
```

The size and `TRAN` column tell the card apart from the PC's own disk. The
device name depends on how the card is attached:

- **The PC's built-in card slot:** `/dev/mmcblk0`, partition `/dev/mmcblk0p1`
  (note the `p`).
- **A USB card reader:** `/dev/sda` (or `sdb`, ...), partition `/dev/sda1`.

For the built-in slot:

```
sudo umount /dev/mmcblk0p1
sudo wipefs -a /dev/mmcblk0
sudo parted -s /dev/mmcblk0 mklabel msdos mkpart primary ext4 1MiB 100%
sudo dd if=build/tmp/deploy/images/ep930x/ep930x-image-ep930x.rootfs.ext4 of=/dev/mmcblk0p1 bs=4M conv=fsync status=progress
sudo e2fsck -f -y /dev/mmcblk0p1
sudo resize2fs /dev/mmcblk0p1
sync
```

For a USB card reader that shows up as `/dev/sda`:

```
sudo umount /dev/sda1
sudo wipefs -a /dev/sda
sudo parted -s /dev/sda mklabel msdos mkpart primary ext4 1MiB 100%
sudo dd if=build/tmp/deploy/images/ep930x/ep930x-image-ep930x.rootfs.ext4 of=/dev/sda1 bs=4M conv=fsync status=progress
sudo e2fsck -f -y /dev/sda1
sudo resize2fs /dev/sda1
sync
```

If the reader is another device name, replace `sda` in all lines. Then eject
the card (`sudo eject /dev/mmcblk0` or `sudo eject /dev/sda`).

Put the card in the board's slot. At the `RedBoot>` prompt, with the TFTP
server from above running:

```
ip_address -l 192.168.68.100 -h 192.168.68.57
load -v -r -b 0x00080000 zImage-ep9301-olimex.dtb.bin
exec -r 0x800000 -s 0x800000 -c "console=ttyAM0,57600 root=/dev/mmcblk0p1 rootwait"
```

Verified: the kernel finds `mmc0: new SDHC card on SPI`, mounts the ext4 root,
starts init, and the `root@ep930x:~#` prompt appears on the console.

## LVGL over VNC (separate tar.gz file system)

The EP9301/EP9302 has no video output, so the demo renders with LVGL into
memory and serves the picture with LibVNCServer. **Status: built and run on
the board; the picture shows in a VNC viewer and mouse clicks work.**

- `ep930x-lvgl-image` builds only a tar.gz holding two 320x240 RGB565 apps,
  `liblvgl`, `libvncserver` and musl. `lvgl-vnc` is a widget demo (label,
  button, slider, load average); `lvgl-clock` is a digital clock like the one
  in `suziepi-touchscreen/apps/clock` (big `HH:MM`, date below). Both share
  `vnc-display.c` (LVGL + VNC + touch-style pointer). It is not bootable and not part of `ep930x-image`.
- `meta-openembedded/meta-oe` supplies the `lvgl` and `libvncserver` recipes.
  It is pinned in `setup-layers.json`, so run `./setup-layers --destdir .`
  again, and add the line below to an existing `build/conf/bblayers.conf`
  (new build dirs get it from the template):

```
  /path/to/redboot/meta-openembedded/meta-oe \
```

- `lvgl_%.bbappend` and `libvncserver_%.bbappend` trim both libraries (no
  DRM/fbdev/SDL, no zlib/jpeg/TLS/websockets/threads; colour depth 16, 128 KB
  LVGL memory).

Build, from `redboot/`:

```
TEMPLATECONF=$PWD/meta-ep930x/conf/templates/default source poky/oe-init-build-env build
bitbake ep930x-lvgl-image
ls -l tmp/deploy/images/ep930x/ep930x-lvgl-image-ep930x.rootfs.tar.gz
tar -tzf tmp/deploy/images/ep930x/ep930x-lvgl-image-ep930x.rootfs.tar.gz
```

Copy it to the board and unpack it into its own directory (the board runs the
same musl, so the libraries are found with `LD_LIBRARY_PATH`):

```
scp tmp/deploy/images/ep930x/ep930x-lvgl-image-ep930x.rootfs.tar.gz root@192.168.68.84:/tmp/
ssh root@192.168.68.84 'mkdir -p /opt/lvgl && tar -xzf /tmp/ep930x-lvgl-image-ep930x.rootfs.tar.gz -C /opt/lvgl'
ssh -L 5900:localhost:5900 root@192.168.68.84 'LD_LIBRARY_PATH=/opt/lvgl/usr/lib /opt/lvgl/usr/bin/lvgl-vnc'
```

The last command keeps the tunnel and the demo running. On the PC connect a
VNC viewer to `localhost:5900` (for example `vncviewer localhost:5900`). The
server listens on the board's loopback address only, so the tunnel is the way
in. If the first command prints a missing library, list `/opt/lvgl/usr/lib`.

Run the clock instead of the demo (`-rfbport` picks another port, so both can
run at once):

```
ssh -L 5900:localhost:5900 root@192.168.68.84 'LD_LIBRARY_PATH=/opt/lvgl/usr/lib /opt/lvgl/usr/bin/lvgl-clock'
```

The clock shows the board's local time. The image has no time zone database,
so it is UTC unless `TZ` is set (for example `TZ=CET-1CEST` before the
command, POSIX format). The time itself comes from NTP (`ep930x-ntpd`). The
digits are a DejaVu Sans Mono Bold subset embedded in `digits_font.c`
(regenerate with `make_font.sh`); LVGL draws them with `tiny_ttf`, enabled by
`lvgl_%.bbappend`.

Try the apps on the PC first, without the board: QEMU user-mode emulation
runs the ARM binaries directly (there is no EP93xx machine in QEMU, so this is
not a board emulation). The tarball holds the musl loader and all the
libraries. Verified with the clock:

```
sudo apt install qemu-user
mkdir -p ~/lvgl-root
tar -xzf build/tmp/deploy/images/ep930x/ep930x-lvgl-image-ep930x.rootfs.tar.gz -C ~/lvgl-root
qemu-arm -L ~/lvgl-root -E LD_LIBRARY_PATH=$HOME/lvgl-root/usr/lib ~/lvgl-root/usr/bin/lvgl-clock
```

Connect a VNC viewer to `localhost:5900` (no ssh tunnel: QEMU shares the PC's
network and the app listens on the loopback address). This checks layout,
fonts and the mouse, not speed: the PC's CPU runs the code, and the demo's
load label and the clock show the PC's load and time.

Gotchas:

- LVGL 9.1 has no `LV_TICK_CUSTOM`; the app must register a clock with
  `lv_tick_set_cb()`. Without it LVGL's time stays 0, its timers never run and
  the pointer is never read, so clicks are silently lost.
- `lv_conf.h` is installed in `include/lvgl/`, so the app compiles with
  `-I${STAGING_INCDIR}/lvgl`.
- The demo treats the mouse as a touch panel: only the left button counts, and
  the pointer moves only while it is pressed.
- TigerVNC 1.13 has no zoom option, so the 320x240 picture is small on a
  high-resolution screen. Remmina works (verified: `remmina -c
  vnc://localhost:5900`, picture and mouse; set Scaling to "Scaled" in the
  profile to follow the window size). RealVNC's `vncviewer -Scaling=FitAuto
  localhost:5900` should also scale (not tried). On the Remmina console it
  prints harmless-looking `Gtk-CRITICAL gtk_widget_get_display` warnings.
