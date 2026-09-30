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

### Recommended: VNC display, serial console in the terminal

```bash
runqemu linuxfb-image slirp nographic publicvnc
```

Then point a VNC viewer at `<build-host>:5900` (display `:0`).

- **`publicvnc`** makes QEMU serve the VM's screen over VNC on port 5900.
  This works the same on a local desktop, over SSH, or in a Codespace, and
  avoids the quirks of an SDL window over X11 forwarding (see below).
- **`nographic`** keeps the serial console in the terminal you ran `runqemu`
  from. You get boot messages and a `root` login there, with no extra window.
  The VM still has its virtio-gpu, so the app still draws to `/dev/fb0` and
  shows up over VNC.
- **`slirp`** uses user-mode networking, so `runqemu` doesn't need `sudo` to
  set up a tap device.
- **Don't pass the machine name** (`runqemu qemuarm64 linuxfb-image`). It
  fails with `IMAGE_LINK_NAME wasn't set to find corresponding
  .qemuboot.conf file`. With an explicit machine, scarthgap's runqemu runs
  `bitbake -e` without the image as target and caches that result, so it
  never learns the image's file name. `MACHINE` comes from `local.conf`
  anyway.

Once boot finishes, the demo fills the screen: Qt version, the platform
(`linuxfb`) and screen size (1280x800), a ticking clock and a click counter.
The mouse and keyboard work through QEMU's USB tablet and keyboard, which Qt
reads via libinput. The app is controlled from the serial console with
`/etc/init.d/linuxfb-demo {start|stop|restart}`.

**VNC notes:**
- **No password:** the VNC server listens on all interfaces. Use it only
  on a trusted network, or tunnel it:
  `ssh -L 5900:localhost:5900 <build-host>`, then connect the viewer to
  `localhost:5900`.
- **One VM per host:** the port is fixed at 5900, so a second
  `publicvnc` VM on the same host fails to start.
- **Browser access:** pair it with `codespace-qt/scripts/novnc-bridge.sh 0`
  to reach it from a browser (e.g. in a Codespace).

### Serial console keys (`nographic`)

The terminal is shared between the VM's serial port and the QEMU monitor.
The escape key is **Ctrl+A**:

| Keys             | Action                                          |
|------------------|-------------------------------------------------|
| Ctrl+A, then X   | quit QEMU immediately (like pulling the plug)   |
| Ctrl+A, then C   | switch between the serial console and the QEMU monitor (`(qemu)` prompt) |
| Ctrl+A, then H   | list these keys                                 |
| Ctrl+A, then A   | send a literal Ctrl+A to the VM                 |

For a clean shutdown, run `poweroff` in the VM instead.

### SDL window (local desktop)

```bash
runqemu linuxfb-image slirp
```

With no display option, runqemu opens an SDL window for the VM's screen.
The serial console goes to a second, hidden SDL window, not the terminal.
The hotkeys use the **left** Ctrl and Alt keys:

| Keys           | Action                                               |
|----------------|------------------------------------------------------|
| Ctrl+Alt+2     | show/hide the serial console window (boot log, `root` login) |
| Ctrl+Alt+G     | grab/release mouse and keyboard; the title bar says when they are grabbed |
| Ctrl+Alt+F     | toggle fullscreen                                    |
| Ctrl+Alt+U     | restore the window to the VM's screen size           |

- **Mouse:** the VM uses an absolute USB tablet, so the pointer normally
  moves in and out of the window without being grabbed. If input does get
  captured, Ctrl+Alt+G releases it.
- **Ctrl+Alt+1 does nothing:** the display window is always open.
- **Quitting:** close the display window, run `poweroff` on the serial
  console, or open the monitor (Ctrl+A, then C in the serial window) and
  type `quit`.
- **"Display output is not active":** over SSH X11 forwarding (`ssh -X`),
  the display window can show this message on a black screen even though
  the app is running. Restarting the app from the serial console
  (`/etc/init.d/linuxfb-demo restart`) may bring the screen back. When
  working remotely, prefer the VNC mode above.

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
