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
`192.168.0.20`, MAC `00:00:00:00:54:33` (set by RedBoot; a mainline kernel
does not read it). About 1 MB of flash is free (`0x60EC0000`-`0x60FC0000`);
the 6.12 `zImage` will not fit the 1.5 MB `zImage` slot, so load it into
RAM while developing.

## Design

- **Appended DTB.** Mainline ep93xx is devicetree-only, but RedBoot passes
  only ATAGS. The DTB is appended to `zImage` (`KERNEL_DEVICETREE_BUNDLE`),
  and the kernel takes memory and the command line from the ATAGS.
- **musl, soft-float.** ARMv4T, no FPU.
- **Root on USB.** `root=/dev/sda1 rootdelay=10` as in the quickstart guide.

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
not `ttyAMA0`. No root filesystem is attached yet.

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
