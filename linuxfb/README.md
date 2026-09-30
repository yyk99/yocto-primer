# Qt6 on the Linux framebuffer (linuxfb) under QEMU

A Yocto build of a small `qemuarm64` (aarch64) image that boots straight into
a Qt6 Widgets app drawn on `/dev/fb0` through Qt's `linuxfb` platform plugin.
There is no X11, Wayland, OpenGL or window manager.

| Layer             | Branch      | Why                                       |
|-------------------|-------------|-------------------------------------------|
| poky              | `scarthgap` | Yocto 5.0 LTS, same as `ch01/`            |
| meta-openembedded | `scarthgap` | `meta-oe` + `meta-python`, needed by meta-qt6 |
| meta-qt6          | `6.11`      | Qt 6.11.x, the current open-source Qt LTS |
| meta-linuxfb      | (this repo) | the app, the image and the build config   |

The exact revisions are pinned in `setup-layers.json`.

## Layout

```
linuxfb/
├── setup-layers, setup-layers.json    check out the pinned upstream layers
└── meta-linuxfb/
    ├── conf/layer.conf
    ├── conf/templates/default/        TEMPLATECONF: local.conf + bblayers.conf
    ├── recipes-qt/linuxfb-demo/       the Qt6 app (CMake) + its init script
    └── recipes-core/images/linuxfb-image.bb
```

`poky/`, `meta-openembedded/`, `meta-qt6/` and `build/` are created here by the
steps below and are gitignored.

## Host prerequisites

Ubuntu 24.04 packages (the Yocto scarthgap list):

```bash
sudo apt install build-essential chrpath cpio debianutils diffstat file gawk \
    gcc git iputils-ping libacl1 locales lz4 python3 python3-git \
    python3-jinja2 python3-pexpect python3-pip python3-subunit socat \
    texinfo unzip wget xz-utils zstd
```

**Ubuntu 23.10+ blocks BitBake by default.** BitBake uses unprivileged user
namespaces to cut tasks off from the network, and AppArmor restricts them, so
every `bitbake` run stops with:

```
ERROR: User namespaces are not usable by BitBake, possibly due to AppArmor.
```

Either allow them for BitBake only with an AppArmor profile:

```bash
sudo tee /etc/apparmor.d/bitbake <<'EOF'
abi <abi/4.0>,
include <tunables/global>
profile bitbake /**/bitbake/bin/bitbake flags=(unconfined) {
  userns,
}
EOF
sudo apparmor_parser -r /etc/apparmor.d/bitbake
```

or lift the restriction system-wide (until reboot):
`sudo sysctl kernel.apparmor_restrict_unprivileged_userns=0`.

**Disk and time.** With `rm_work` (on by default here) expect roughly 35-45 GB
for `build/`, including `downloads/` and `sstate-cache/`. A cold build takes
several hours on 8 cores. Most of that is the toolchain, qtbase and ICU.

## Build

All commands run from `linuxfb/`.

```bash
./setup-layers --destdir .     # clone poky, meta-openembedded, meta-qt6 at the pinned revs
TEMPLATECONF=$PWD/meta-linuxfb/conf/templates/default source poky/oe-init-build-env build
bitbake linuxfb-image
```

- **Pass `--destdir .`.** Without it, `setup-layers` clones next to the repo
  root rather than into `linuxfb/`.
- **`TEMPLATECONF` only matters the first time,** when `build/conf/` doesn't
  exist yet. Later shells just run `source poky/oe-init-build-env build`. To
  pick up template changes, delete `build/conf/` and source again with
  `TEMPLATECONF` set.
- **poky's `./setup-build` helper doesn't list this template.** It only
  scans the layers `setup-layers` checked out, not `meta-linuxfb`, so use
  `TEMPLATECONF` as shown.

To build just the app (e.g. while editing `main.cpp`): `bitbake linuxfb-demo`.

## Run

```bash
runqemu qemuarm64 linuxfb-image slirp
```

- `slirp` uses user-mode networking, so `runqemu` doesn't need `sudo` to set
  up a tap device.
- QEMU opens an SDL window. Once boot finishes, the demo fills it: Qt
  version, the platform (`linuxfb`) and screen size, a ticking clock and a
  click counter. The mouse and keyboard work through QEMU's USB tablet and
  keyboard, which Qt reads via libinput.
- Log in as `root` with no password on the serial console (the terminal you
  ran `runqemu` from). The app is controlled with
  `/etc/init.d/linuxfb-demo {start|stop|restart}`.
- **No local display** (SSH session, Codespace): add `publicvnc` for a VNC
  server on port 5900, or `nographic` for serial only. `publicvnc` pairs with
  `codespace-qt/scripts/novnc-bridge.sh 0` to reach it from a browser.

## How the pieces fit

- **Display.** runqemu gives `qemuarm64` a `virtio-gpu-pci` device.
  linux-yocto's qemu kernel config enables `DRM_VIRTIO_GPU` and
  `DRM_FBDEV_EMULATION`, which gives a `/dev/fb0` without any kernel changes.
- **Qt.** `DISTRO_FEATURES:remove = "x11 wayland opengl vulkan ptest"` in
  `local.conf` makes meta-qt6 build qtbase with `no-opengl` and
  `QT_QPA_DEFAULT_PLATFORM = "linuxfb"`. It also keeps mesa out of the build
  entirely; the image pulls in about 270 recipes.
- **App.** `linuxfb-demo` uses Qt Widgets only, so qtbase is the only Qt
  module it depends on (no qtdeclarative/QML). It depends on
  `ttf-bitstream-vera` because Qt6 no longer ships fonts.
- **Startup.** A sysvinit script (poky's default init) starts the app in
  runlevel 5 with `QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0`.

## Updating the pinned layers

`setup-layers` is poky's `scripts/oe-setup-layers`, unmodified. Don't
hand-edit it. `setup-layers.json` is maintained by hand: to move a layer
forward, update its `rev` (and `describe`) to the new commit on the same
branch, then re-run `./setup-layers --destdir .`. If you switch meta-qt6 to
another Qt branch, check that its `conf/layer.conf` still lists `scarthgap` in
`LAYERSERIES_COMPAT_qt6-layer`.

`bitbake-layers create-layers-setup` isn't used here, because it would also
record this repo itself as a layer source.
