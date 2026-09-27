# Running QEMU with graphics in a GitHub Codespace

A Codespace is a container with no physical display and no GPU. `qemu-system-*`
by default tries to open a local window (SDL or GTK), which fails with
something like:

```
Could not initialize SDL(x11 not available) - exiting
```

This is exactly what you hit if you build a Yocto image with a graphical
target (e.g. `core-image-sato`) and run `runqemu qemux86-64` without
`nographic`: `runqemu` shells out to `qemu-system-x86_64` the same way.

There is no way to get a literal local window out of a remote container, but
you can get the guest's actual framebuffer into your browser or your local
VNC viewer. Two ways to do that, in order of how much you're trying to see:

1. **Point-to-point VNC** — QEMU serves its own display over VNC; you tunnel
   just that one port. Minimal, no desktop environment involved. Best when
   you only care about the QEMU guest's screen.
2. **`desktop-lite` devcontainer feature** — gives the whole Codespace a
   small Fluxbox desktop reachable via noVNC in the browser. Then you run
   `runqemu`/`qemu-system-*` normally *inside* that desktop and its window
   just appears. Best when you want a real desktop (multiple windows, a
   terminal next to the VM, etc.).

Both rely on VNC under the hood — QEMU has no other remote-display option
that works through a container boundary.

## KVM acceleration: check, don't assume

`-enable-kvm` needs `/dev/kvm`, which needs nested virtualization on the
Codespace's underlying VM host. This has been inconsistent across Codespaces
base images — some variants expose it, some don't (see
`devcontainers/images` issue #884, where the Jammy variant image had no
`/dev/kvm` while Focal did). Always check rather than hard-coding
`-enable-kvm`:

```bash
scripts/check-kvm.sh
```

If it's not available, drop `-enable-kvm` (or pass `-machine accel=tcg`)
and expect QEMU to run in pure software emulation — much slower, but it
still boots and still displays fine.

## Option 1 — direct VNC, no desktop

`scripts/run-qemu-vnc.sh` boots a disk image (or a Yocto `runqemu`-built
image directly with `qemu-system-x86_64`) with its display exported as VNC
display `:1` (TCP port 5901), auto-detecting KVM:

```bash
scripts/run-qemu-vnc.sh /path/to/core-image-sato-qemux86-64.wic
```

To reach port 5901 from your machine, VNC is a raw TCP protocol, so the
Codespaces web Ports view (which is really an HTTP proxy) can't forward it
directly — you need one of:

- **VS Code Desktop**, connected to the Codespace: forward port 5901 (Ports
  panel → Forward a Port), then point any VNC viewer (TigerVNC, Remmina, the
  macOS "Screen Sharing" app, ...) at `localhost:5901`.
- **`gh` CLI from a local terminal**, no VS Code needed:
  ```bash
  gh codespace ports forward 5901:5901 -c <codespace-name>
  ```
  then connect a VNC viewer to `localhost:5901`.
- **Plain SSH tunnel**, if you've set up Codespaces SSH config:
  ```bash
  ssh cs.<codespace-name>.<branch> -L 5901:localhost:5901 -N
  ```

## Option 2 — noVNC bridge, browser only

If you don't want to install a VNC viewer, or you're stuck on the web
version of Codespaces (which only forwards HTTP), `scripts/novnc-bridge.sh`
puts `websockify`+`noVNC` in front of QEMU's VNC port and serves a plain web
page:

```bash
scripts/run-qemu-vnc.sh /path/to/image.wic &
scripts/novnc-bridge.sh        # serves the noVNC client on :6080
```

Then forward port 6080 in the Ports panel (works from the web IDE too,
since it's plain HTTP/WebSocket) and open it — the Ports panel's globe icon
does this for you. noVNC shows a "Connect" page; click it and you're looking
at the guest's screen.

## Option 3 — full desktop via `desktop-lite`

`.devcontainer/devcontainer.json` in this folder adds the
[`desktop-lite`](https://github.com/devcontainers/features/tree/main/src/desktop-lite)
feature, which installs Fluxbox + TigerVNC + noVNC into the container itself
and forwards port 6080:

```jsonc
{
  "image": "mcr.microsoft.com/devcontainers/base:ubuntu",
  "features": {
    "ghcr.io/devcontainers/features/desktop-lite:1": {}
  },
  "forwardPorts": [6080],
  "portsAttributes": { "6080": { "label": "desktop (noVNC)" } }
}
```

Rebuild the container, open port 6080 in the browser, log in with the
default password `vscode`, open a terminal *inside that desktop* (or use the
one VS Code opens there automatically), and just run:

```bash
qemu-system-x86_64 -enable-kvm -m 2048 <...>
# or: runqemu qemux86-64
```

No `-vnc` flag needed this time — QEMU opens its normal SDL/GTK window,
which now has an actual X server (Fluxbox) to open it on.

**Smoke-test the desktop before bothering with QEMU at all**: this pipeline
(noVNC → Fluxbox → X server) works independently of QEMU, and `apt` on the
container can install a plain X11 app to prove it — no Yocto build, no
image, no VM:

```bash
scripts/desktop-lite-smoke-test.sh
```

It installs `x11-apps` (for `xeyes`) if missing and launches it; if it shows
up in the noVNC desktop, the whole rendering path is confirmed and any
QEMU window will show up the same way. This only works for apps installed
*on the container itself* — `apt` has no reach into a QEMU guest's
filesystem, which is a separate OS image with its own package manager
(`opkg` by default for Yocto images).

## Which one to use

- Quick look at a Yocto image's boot/GUI, nothing else needed → **Option 1**.
- Same, but you're on the web IDE or don't want a VNC client → **Option 2**.
- Doing real desktop work, multiple windows, want a persistent environment
  → **Option 3**, accepting the extra container weight.

## References

- [GitHub Codespaces: Developing GUI Applications — Tiago Pascoal](https://pascoal.net/2024/01/28/codespaces-developing-gui-apps-2/)
- [Running QEMU Virtual Machine in GitHub Codespaces](https://chun.itcdt.top/2024/07/Running-QEMU-Virtual-Machine-in-Github-Codespaces/)
- [`devcontainers/features` — `desktop-lite`](https://github.com/devcontainers/features/tree/main/src/desktop-lite)
- [`devcontainers/images` #884 — KVM missing on the Jammy variant](https://github.com/devcontainers/images/issues/884)
