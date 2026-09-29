# CLAUDE.md

Guidance for Claude Code when working in this repository.

## Repo structure

- `ch01/` - Yocto Project + GitHub primer content (core concepts, layers,
  CI practices, MACHINE selection, cleanup).
- `codespace-qt/` - how to build and run QEMU with graphical output inside
  a GitHub Codespace (a headless container: no physical display, no GPU).
  `codespace-qt/README.md` has the full writeup; below are gotchas from
  building it that aren't obvious from just reading the scripts.

## codespace-qt gotchas

- **devcontainer.json location matters.** GitHub Codespaces only discovers
  alternate dev container configs under root `.devcontainer/<name>/`. A
  `devcontainer.json` placed inside `codespace-qt/.devcontainer/` is
  invisible to the Codespaces "create codespace" UI. The working one lives
  at `.devcontainer/codespace-qt/devcontainer.json`.
- **KVM in Codespaces/devcontainers is inconsistent** across base images
  (e.g. Jammy vs Focal variants have differed on whether `/dev/kvm` exists
  at all) - always check with `codespace-qt/scripts/check-kvm.sh` rather
  than hardcoding `-enable-kvm`.
- **Yocto's prebuilt image filenames vary by release.** Some releases use a
  `.rootfs` infix before the format extension; compression has moved from
  `.bz2`/`.gz` to `.zst` over time. `fetch-core-image-sato.sh` scrapes the
  actual `downloads.yoctoproject.org` directory listing rather than
  hardcoding names - don't hardcode them elsewhere either.
- **`qemux86-64` images are a kernel + ext4 rootfs pair, not a `.wic` disk
  image.** `run-qemu-vnc.sh` (whole-disk `.wic`, e.g. for a real-hardware
  machine like `genericx86-64`) and `run-core-image.sh` (kernel + rootfs +
  `.qemuboot.conf`, e.g. for `qemux86-64`) are not interchangeable.
- **`codespace-qt/downloads/` is gitignored.** It's
  `fetch-core-image-sato.sh`'s output directory (multi-hundred-MB images) -
  regenerate with the script rather than committing images.
- **The decompressed rootfs can be deleted to save disk space.**
  `run-core-image.sh` auto-decompresses the `.zst`/`.bz2`/`.gz` source on
  demand if the plain file is missing, so keeping just the compressed copy
  at rest is safe.
- **`core-image-sato`'s pointer is invisible by design**, not a bug in these
  scripts - Matchbox/Sato is a touchscreen UI and ships a blank cursor
  theme; clicks can work fine with nothing visibly rendered.
