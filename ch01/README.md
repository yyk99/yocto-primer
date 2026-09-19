# Yocto and GitHub Primer

## Yocto in a nutshell

The Yocto Project is a build system for creating custom embedded Linux distributions. You describe what you want in metadata, and it builds a full image (bootloader, kernel, rootfs, SDK) from source.

**Core concepts**
- **BitBake** is the build engine. It parses metadata and runs tasks (fetch, unpack, patch, configure, compile, install, package).
- **Recipes (`.bb`)** describe how to build one piece of software: source URI, checksums, dependencies, build steps.
- **Bbappend (`.bbappend`)** extends an existing recipe without forking it, e.g. adding a patch or config.
- **Layers** are directories of related metadata (`meta-*`). Examples: `poky` (reference distro and OE-Core), `meta-openembedded`, BSP layers like `meta-raspberrypi`, and your own `meta-mycompany`.
- **Machine (`MACHINE`)** is the target hardware. **Distro (`DISTRO`)** is the policy and feature set. **Image recipe** defines the rootfs contents.
- **Config files**: `conf/local.conf` (build settings) and `conf/bblayers.conf` (which layers are active).
- **sstate cache and downloads (`DL_DIR`)** are what make rebuilds fast. Sharing them matters a lot in CI.

**Minimal workflow**
```bash
git clone -b scarthgap git://git.yoctoproject.org/poky
cd poky
source oe-init-build-env build
# edit conf/local.conf: MACHINE = "qemux86-64"
bitbake core-image-minimal
runqemu qemux86-64
```

Use a stable release branch (e.g. `scarthgap`, an LTS) and keep all layers on the same branch.

**Writing a custom layer**
```bash
bitbake-layers create-layer ../meta-myproject
bitbake-layers add-layer ../meta-myproject
```
Put your recipes in `recipes-<category>/<name>/<name>_<ver>.bb`, and custom image or machine config in the layer rather than in `local.conf`.

## Using GitHub with Yocto

**Repo strategy**: pick one:
1. **One repo per layer**: your `meta-myproject` is its own repo; upstream layers stay upstream.
2. **A manifest or config repo** that pins all layers and versions. This is the usual best practice, using either:
   - **`kas`** (a YAML file defines repos, branches or commits, machine, distro, targets; works well in CI), or
   - **Google `repo`** (XML manifest), or
   - **Git submodules** (simpler but clunkier).

**Example `kas` file (`kas.yml`)**
```yaml
header:
  version: 14
machine: qemux86-64
distro: poky
target: core-image-minimal
repos:
  poky:
    url: https://git.yoctoproject.org/poky
    branch: scarthgap
    layers:
      meta:
      meta-poky:
  meta-myproject:
    path: .
```
Then `kas build kas.yml` reproduces the whole build.

**Recommended practices**
- Pin upstream layers to commit SHAs (or at least release branches) for reproducibility.
- Never commit `build/`, `tmp/`, `sstate-cache/`, or `downloads/`; add them to `.gitignore`.
- Keep `local.conf` settings that matter in the layer or kas file, not hand-edited per machine.
- Use `SRC_URI = "git://github.com/org/repo;protocol=https;branch=main"` with `SRCREV` pinned to a commit. Avoid `AUTOREV` in release builds.

**GitHub Actions CI**
- Yocto builds are heavy (hours cold, 100+ GB disk), and hosted runners are small. Use self-hosted runners or a big-runner tier for full builds.
- Cache `DL_DIR` and `SSTATE_DIR` (via `actions/cache`, or better, a shared mirror via `SSTATE_MIRRORS` and `SOURCE_MIRROR_URL`).
- Run cheap checks on every PR (`yocto-check-layer`, `bitbake -p` parse check, `oelint-adv` linting) and full image builds on merge or nightly.
- Use the `crops/poky` container (or `kas-container`) for a consistent build environment.

**Typical PR flow**: feature branch, then change a recipe or bbappend, then CI parse and lint, then optional test build, then review, then merge. Tag releases so an image can be traced to exact metadata.

## Common gotchas
- Mixing layer branches (e.g. `kirkstone` with `scarthgap`) breaks parsing.
- Missing `LICENSE` or `LIC_FILES_CHKSUM` in recipes causes errors.
- Don't edit files inside `tmp/work`. Patch via `devtool` or recipe patches instead.
- `devtool modify <recipe>` is the easiest way to iterate on source and generate patches.

I can go deeper on any piece, such as writing a recipe, setting up the GitHub Actions workflow, or structuring a kas config for multiple machines. Which is closest to what you're doing?
