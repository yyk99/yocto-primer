# U-Boot primer: hands-on U-Boot in QEMU, built with Yocto

A Yocto build of a small `qemuarm64` (aarch64) system where **U-Boot is the
firmware**. QEMU's `virt` board runs U-Boot straight out of emulated **NOR
flash**, like a real SoC board, and U-Boot loads the Linux kernel from the
flash or from a FAT partition on a virtual disk. Its environment and a small
JFFS2 filesystem live in a second flash bank. You get a real U-Boot prompt to poke at, and a build you can
change and re-run in minutes.

| Layer      | Branch      | Why                                          |
|------------|-------------|----------------------------------------------|
| poky       | `scarthgap` | Yocto 5.0 LTS, same as `ch01/`; ships U-Boot 2024.01 |
| meta-uboot | (this repo) | the machine, the image, the U-Boot tweaks    |

The exact poky revision is pinned in `setup-layers.json`.

## Layout

```
u-boot-primer/
├── setup-layers, setup-layers.json    check out the pinned poky
├── scripts/run-qemu.sh                boot U-Boot + the disk image in QEMU
├── jffs2-root/                        files packed into the JFFS2 image in flash
└── meta-uboot/
    ├── conf/layer.conf
    ├── conf/machine/qemuarm64-uboot.conf   qemuarm64 + U-Boot + wic disk
    ├── conf/templates/default/             TEMPLATECONF: local.conf + bblayers.conf
    ├── recipes-bsp/u-boot/                 bbappend + files/primer.cfg
    ├── recipes-kernel/linux/               bbappend + files/mtd-jffs2.cfg (flash + JFFS2)
    ├── recipes-core/images/uboot-primer-image.bb
    └── wic/uboot-qemu.wks                  FAT /boot (kernel) + ext4 rootfs
```

`poky/` and `build/` are created here by the steps below and are gitignored.

## How the pieces fit

- **Firmware.** `u-boot` is built from poky's recipe with
  `qemu_arm64_defconfig` (poky's `qemuarm64` already selects it). The result,
  `u-boot.bin`, is a flat binary. `files/primer.cfg` is a config fragment
  merged on top.
- **NOR flash.** QEMU's `virt` has two CFI NOR flash banks, 64 MiB each.
  `run-qemu.sh` builds `build/flash/flash0.img` with `u-boot.bin` at the start
  (the CPU starts at address 0, so this is where U-Boot runs from) and the
  kernel at `0x400000`. Bank 1 (`flash1.img`, mapped at `0x04000000`) holds the
  U-Boot environment in its first 1 MiB: `qemu_arm64_defconfig` sets
  `CONFIG_ENV_IS_IN_FLASH` with `CONFIG_ENV_ADDR=0x4000000`, so `saveenv`
  survives a restart. A JFFS2 filesystem, built from `jffs2-root/`, starts at
  `0x04100000`. Each bank is 64 MiB in 256 KiB erase sectors (the geometry
  `mkfs.jffs2 -e 0x40000` must match). The two chips are interleaved on a
  32-bit bus; `primer.cfg` sets `CONFIG_SYS_FLASH_CFI_WIDTH_32BIT` so U-Boot
  sees the full 64 MiB (by default it probes 16 bits and sees 32 MB).
- **Device tree.** QEMU generates a DTB describing the `virt` board and hands
  it to U-Boot; U-Boot's own copy is at `$fdtcontroladdr`. Nothing here builds
  a DTB.
- **Disk.** The root filesystem lives on a disk, like an SD card or eMMC on a
  real board. `uboot-qemu.wks` makes a two-partition MBR disk. U-Boot sees it as
  `virtio 0`: partition 1 is FAT with the kernel `Image`, partition 2 is the
  ext4 root filesystem.
- **No `-kernel`, no `-bios`.** Unlike `runqemu`, `run-qemu.sh` never passes
  the kernel or the firmware to QEMU as such. The flash images are the only
  way they get in; if the kernel boots, it is because U-Boot loaded it.

## Host prerequisites

Same as the other chapters (Ubuntu 24.04, Yocto scarthgap list):

```bash
sudo apt install build-essential chrpath cpio debianutils diffstat file gawk \
    gcc git iputils-ping libacl1 locales lz4 python3 python3-git \
    python3-jinja2 python3-pexpect python3-pip python3-subunit socat \
    texinfo unzip wget xz-utils zstd
```

Ubuntu 23.10+ also needs BitBake allowed to use unprivileged user namespaces,
or every `bitbake` run stops with `User namespaces are not usable by
BitBake`. Quickest fix, until reboot:
`sudo sysctl kernel.apparmor_restrict_unprivileged_userns=0`.

A QEMU on the host (`sudo apt install qemu-system-arm`) is the easiest way to
run the result; otherwise `bitbake qemu-system-native` and the script finds
that one.

**Disk and time.** Roughly 15-20 GB with downloads and sstate. A cold build
takes about an hour or two on 8 cores: toolchain, kernel, then the image.
U-Boot itself builds in a minute or so.

## Build

All commands run from `u-boot-primer/`.

```bash
./setup-layers --destdir .     # clone poky at the pinned rev
TEMPLATECONF=$PWD/meta-uboot/conf/templates/default source poky/oe-init-build-env build
bitbake uboot-primer-image
```

- **Pass `--destdir .`.** Without it, `setup-layers` clones next to the repo
  instead of inside `u-boot-primer/`.
- **`TEMPLATECONF`** is how the build picks up `meta-uboot`'s `local.conf`
  and `bblayers.conf`; it only matters the first time, when `build/conf/` is
  created. Later shells just `source poky/oe-init-build-env build`.
- The outputs land in `build/tmp/deploy/images/qemuarm64-uboot/`:
  `u-boot.bin` and `uboot-primer-image-qemuarm64-uboot*.wic`.

Just the bootloader, no kernel or rootfs (handy while experimenting):

```bash
bitbake u-boot
```

## Run

```bash
scripts/run-qemu.sh
```

The script uses a host `qemu-system-aarch64` if there is one, else the one
`bitbake qemu-system-native` built. It regenerates `flash0.img` from the build
every run, so a rebuilt U-Boot or kernel is picked up. The disk is a snapshot,
so changes to it are discarded on exit; `PERSIST=1 scripts/run-qemu.sh` writes
to the image. `flash1.img` (the saved environment and the JFFS2 data) is kept
between runs; recreate it, discarding both, with
`RESET_ENV=1 scripts/run-qemu.sh`. Building the JFFS2 image needs
`mkfs.jffs2`: `bitbake mtd-utils-native` (or `sudo apt install mtd-utils`);
without it `flash1.img` is created without a filesystem. The script also works
with only `bitbake u-boot` built, with no kernel or disk. Quit QEMU with
`Ctrl-a x`.

You will see the banner, with `-primer` in the version string, and a 5 second
`Hit any key to stop autoboot` countdown. The image has no `boot.scr` or
`extlinux.conf`, so if you let it run, U-Boot's default distro boot finds
nothing to boot (it also tries PXE/DHCP over the network) and drops to the
`=>` prompt. That is the expected starting point for the exercises.

## Exercises

Type these at the `=>` prompt.

### 1. Look around

```
version
help
bdinfo
printenv
```

- `bdinfo` shows RAM start and size, and where U-Boot relocated itself.
- In `printenv`, find `kernel_addr_r`, `fdt_addr_r`, `ramdisk_addr_r` and
  `bootcmd`. Those addresses are where you put things in the next exercise.

### 2. Boot Linux by hand

Find the disk, then load and boot the kernel the way `bootcmd` would:

```
virtio scan
virtio info
part list virtio 0
ls virtio 0:1
load virtio 0:1 ${kernel_addr_r} Image
setenv bootargs "root=/dev/vda2 rw console=ttyAMA0"
booti ${kernel_addr_r} - ${fdtcontroladdr}
```

- `load` needs the interface (`virtio`), the `device:partition` (`0:1`), an
  address and a file name. `booti` is the aarch64 `Image` loader; the `-`
  means no initramfs, and the last argument is the DTB.
- Root login has no password (`debug-tweaks`). `poweroff` ends the QEMU run.
- Try wrong values and read the errors: `root=/dev/vda1` (that is the FAT
  partition) panics at mount; leaving out `console=ttyAMA0` gives you a
  silent boot after `Starting kernel ...`.

### 3. Make it boot itself

Put the whole sequence in `bootcmd`, then boot:

```
setenv bootcmd 'load virtio 0:1 ${kernel_addr_r} Image; booti ${kernel_addr_r} - ${fdtcontroladdr}'
setenv bootargs 'root=/dev/vda2 rw console=ttyAMA0'
boot
```

Then `saveenv` writes the environment to flash bank 1. It takes about 30
seconds and prints nothing while it writes (QEMU's flash is programmed word by
word), so wait for the prompt. The values are
still there after `reset`, and across `run-qemu.sh` runs. Undo it with
`env default -a; saveenv`, or start with `RESET_ENV=1`. To bake values into
the build instead, see exercise 9.

### 4. The NOR flash

```
flinfo
md.b 0x400000 20
cp.b 0x400000 ${kernel_addr_r} 0x2000000
booti ${kernel_addr_r} - ${fdtcontroladdr}
```

- `flinfo` shows the two flash banks (64 MB each, 256 sectors of 256 KiB) and
  which sectors are protected (`RO` is U-Boot itself, in the sectors it runs
  from).
- The kernel is in flash at `0x400000`. `cp.b` copies it to RAM (32 MiB is
  more than its size), then `booti` boots it as before.
- To program flash, erase a sector first (flash can only change 1 bits to 0):

```
protect off 0x800000 +0x40000
erase 0x800000 +0x40000
mw.l ${loadaddr} 0xcafef00d
cp.b ${loadaddr} 0x800000 4
md.l 0x800000 1
```

  `flash0.img` is rebuilt from the build on every run, so this does not last
  (and, unlike the environment in bank 1, is meant not to).

### 5. A JFFS2 filesystem in flash

`run-qemu.sh` packs `jffs2-root/` into a JFFS2 image at `0x04100000` of
bank 1 when it creates `flash1.img`. Tell U-Boot where it is, select it, and
read it:

```
setenv mtdids nor1=nor1
setenv mtdparts mtdparts=nor1:1m(env),-(jffs2)
mtdparts
chpart nor1,1
fsinfo
fsls
fsls /notes
fsload ${loadaddr} /hello.txt
md.b ${loadaddr} 24
```

- `mtdids` names the flash device and `mtdparts` splits bank 1 into a 1 MiB
  `env` partition (where `saveenv` writes) and a `jffs2` partition with the
  rest. `chpart nor1,1` selects the second one.
- The JFFS2 commands are `fsinfo`, `fsls` and `fsload`. (`ls` is the generic
  filesystem command, which does not know JFFS2.)
- Run `saveenv` after the two `setenv` lines to keep them across restarts.
  The environment (bank 1, first 1 MiB) and the filesystem (from `0x04100000`)
  don't overlap, so the files are still readable after `saveenv` and `reset`.
- Change the files under `jffs2-root/`, then `RESET_ENV=1 scripts/run-qemu.sh`
  to rebuild the image and see your files.
- U-Boot's JFFS2 support is read-only. To write to the filesystem, use Linux
  (exercise 6).

### 6. Use the flash from Linux

The kernel has MTD, CFI flash and JFFS2 support (`files/mtd-jffs2.cfg`), and
the image has `mtd-utils`. Boot it from flash with the flash partitions on the
kernel command line:

```
cp.b 0x400000 ${kernel_addr_r} 0x2000000
setenv bootargs "root=/dev/vda2 rw console=ttyAMA0 mtdparts=0.flash:64m(bank0),1m(env),-(jffs2)"
booti ${kernel_addr_r} - ${fdtcontroladdr}
```

Then, as root in Linux:

```
cat /proc/mtd
mkdir -p /mnt/jffs2
mount -t jffs2 /dev/mtdblock2 /mnt/jffs2
ls -lR /mnt/jffs2
echo "written by Linux" > /mnt/jffs2/linux.txt
sync
umount /mnt/jffs2
poweroff
```

Start QEMU again (keep `flash1.img`: no `RESET_ENV`) and look from U-Boot:

```
setenv mtdids nor1=nor1
setenv mtdparts mtdparts=nor1:1m(env),-(jffs2)
chpart nor1,1
fsls
fsload ${loadaddr} /linux.txt
md.b ${loadaddr} 12
```

- The device tree has one `cfi-flash` node with two `reg` ranges, so Linux
  probes both chips and concatenates them into one 128 MiB device named
  `0.flash` (see the `dmesg` lines with `physmap-flash`). The `mtdparts=`
  on the command line splits it the same way U-Boot did for bank 1: 64 MiB for
  bank 0, 1 MiB environment, the rest JFFS2.
- With these partitions `/proc/mtd` lists `mtd0` to `mtd2`, so the JFFS2
  partition is `/dev/mtdblock2`. If you pick a device that does not exist,
  `mount` fails with `Couldn't look up '/dev/mtdblockN'` and anything you
  then write goes to the root filesystem instead of the flash.
- The erase size Linux reports (`0x40000`) is the one `mkfs.jffs2 -e` used.
- Try `flash_erase` and `flashcp` from `mtd-utils` on `/dev/mtd2`, then
  `mount` it again and see what JFFS2 does with an erased partition.

### 7. Memory and the device tree

```
md.l ${kernel_addr_r} 4              # dump 4 words of RAM
mw.l ${fdt_addr_r} 0x12345678 4      # write them
cmp.l ${kernel_addr_r} ${fdt_addr_r} 4
fdt addr ${fdtcontroladdr}
fdt print /memory
fdt print /chosen
```

`md` / `mw` are the same commands you would use on real hardware to peek at
registers. After `booti`, compare `fdt print /chosen` before and after: U-Boot
adds `bootargs` to the DTB it passes to the kernel.

### 8. Networking

QEMU's user-mode network gives the guest `10.0.2.15` and a DHCP server:

```
dhcp
printenv ipaddr serverip
ping 10.0.2.2
```

`10.0.2.2` is the host as seen from the guest. To go further, QEMU can serve
TFTP itself: change the script's `-netdev user,id=net0` to
`-netdev user,id=net0,tftp=/some/dir`, put a kernel `Image` in that dir, then
`tftpboot ${kernel_addr_r} Image` and `booti` it.

### 9. Change U-Boot's configuration and rebuild

Edit `meta-uboot/recipes-bsp/u-boot/files/primer.cfg`, for example:

```
CONFIG_BOOTDELAY=10
CONFIG_LOCALVERSION="-mine"
```

then:

```bash
bitbake u-boot
scripts/run-qemu.sh      # the banner and the countdown change
```

`bitbake -c savedefconfig u-boot` writes the minimal defconfig of the merged
result to the u-boot work dir (the path is printed), and
`bitbake -c menuconfig u-boot` opens the interactive menu in a new terminal
window. A config option that compiles in a command, such as `CONFIG_CMD_...`,
shows up in `help` after the rebuild.

### 10. Patch U-Boot's source

`devtool` gives you a git checkout of the U-Boot source to edit:

```bash
devtool modify u-boot
# edit workspace/sources/u-boot/..., e.g. add a printf to board/emulation/qemu-arm/qemu-arm.c
bitbake u-boot
scripts/run-qemu.sh
devtool finish u-boot ../meta-uboot     # turns your commits into a patch in the layer
```

Commit in `workspace/sources/u-boot` before `devtool finish`; the commits
become the patch files.

## Troubleshooting

- **Nothing happens after `Starting kernel ...`.** Wrong or missing
  `console=ttyAMA0`, or the DTB address is wrong.
- **`Wrong Image Format for booti command`.** The load failed or used the
  wrong file. Check `ls virtio 0:1` and that `load` printed a byte count.
- **`saveenv` says `Flash buffer write timeout`.** `primer.cfg` turns off
  `CONFIG_SYS_FLASH_USE_BUFFER_WRITE` for this; rebuild `u-boot` if you have
  an older build.
- **`saveenv` seems to hang after `Writing to Flash...`.** It is writing
  256 KiB one word at a time, which takes about 30 seconds. Wait for `done`.
- **`Unknown command 'xyz'`.** The command is not compiled into this U-Boot;
  enable its `CONFIG_CMD_*` in `primer.cfg` (exercise 9).
- **The kernel is skipped as incompatible, or `do_kernel_metadata` says
  `Could not locate BSP definition for qemuarm64-uboot/standard`.**
  `qemuarm64-uboot` must keep `MACHINEOVERRIDES =. "qemuarm64:"` and
  `KMACHINE = "qemuarm64"` in the machine config.
