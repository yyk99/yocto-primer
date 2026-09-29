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
runqemu qemux86-64 nographic
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


## Build for Raspberry Pi 3
Use the `meta-raspberrypi` BSP layer, which provides these machines for the Pi 3:

- `raspberrypi3-64`: 64-bit (aarch64). This is the one to prefer for a Pi 3 or 3B+ on a current release.
- `raspberrypi3`: 32-bit (armv7). Use it if you specifically need a 32-bit userland.

In `conf/local.conf`:

```
MACHINE ?= "raspberrypi3-64"
```

You also need to add the layer, on the same release branch as your poky checkout (e.g. `scarthgap`):

```bash
git clone -b scarthgap https://git.yoctoproject.org/meta-raspberrypi
bitbake-layers add-layer ../meta-raspberrypi
```

`meta-raspberrypi` depends on `meta-oe`, `meta-python`, and `meta-multimedia` from `meta-openembedded`, so those need to be in `bblayers.conf` too. Then build an image such as `core-image-base` or `rpi-test-image`.

To write the image to an SD card, the output is in `build/tmp/deploy/images/raspberrypi3-64/` and ends in `.wic.bz2` (or `.wic.xz`, depending on the release). Flash it with `bmaptool` or `dd`.

The `raspberrypi3` machine names and the exact `IMAGE_FSTYPES` defaults have shifted between releases, so check the layer's README on your branch, and list `conf/machine/` in the layer to confirm the names available.

# How to clean up for a particular MACHINE

Changing `MACHINE` doesn't wipe anything. Everything goes into the same `build/tmp/`, and the machine-specific parts sit side by side. Here is how to clean up, from lightest to heaviest.

**1. Delete `tmp/` (most common)**

```bash
rm -rf build/tmp
```

It's safe because `tmp/` is disposable output. What makes rebuilds fast lives elsewhere, in `sstate-cache/` and `downloads/`, and both survive. After deleting `tmp/`, a rebuild mostly restores from sstate rather than recompiling. This is the standard way to reclaim space.

**2. Remove only the first machine's leftovers**

If you want to keep `tmp/` and drop one machine's outputs:

```bash
rm -rf build/tmp/deploy/images/raspberrypi3-64
rm -rf build/tmp/work/raspberrypi3_64-*      # machine-specific work dirs
```

The exact work directory names vary (they use the machine's tune/arch, and `-` becomes `_`), so list `tmp/work/` first. Shared packages built for a common arch, like `x86-64-v3-poky-linux`, are reused by other machines that share that arch, so don't delete them blindly.

**3. Clean through BitBake**

```bash
bitbake -c cleansstate <recipe>   # one recipe: work dir + its sstate
bitbake -c cleanall <recipe>      # also removes downloaded sources
bitbake -c clean <recipe>         # work dir only
```

`cleansstate` also deletes the cached artifact, so that recipe will fully rebuild next time. `cleanall` is the one that costs you re-downloads, so use it sparingly. You can also run these against an image target, though that only cleans that image recipe itself, not its dependencies.

**4. Avoid the problem: separate build dirs or `TMPDIR`**

```bash
source oe-init-build-env build-rpi3
source oe-init-build-env build-qemu
```

Each has its own `tmp/`, and you can point both at one shared cache by setting in each `local.conf`:

```
DL_DIR = "/home/yyk/yocto-cache/downloads"
SSTATE_DIR = "/home/yyk/yocto-cache/sstate-cache"
```

That gives you independent `tmp/` directories that are trivial to delete, while still sharing downloads and sstate between them.

**5. Reclaim space automatically**

Adding `INHERIT += "rm_work"` to `local.conf` deletes each recipe's work directory after it builds, which cuts disk use dramatically. The trade-off is that you lose the work directories for debugging. You can exclude recipes you're actively working on with `RM_WORK_EXCLUDE += "myrecipe"`.

The `sstate-cache/` directory grows over time. To trim it, use `scripts/sstate-cache-management.py` from poky, or just delete files older than some age.

# what MACHINE for raspberry pi 3 with qemu?

There isn't a dedicated one. Poky's QEMU machines (`qemux86-64`, `qemuarm64`, `qemuarm`, and so on) emulate generic virtual boards, not a Raspberry Pi. And `raspberrypi3-64` from `meta-raspberrypi` builds images for real hardware, which `runqemu` can't boot as-is (it needs a QEMU-specific kernel and machine setup).

**Closest match**

```
MACHINE ?= "qemuarm64"
```

This gives you the same CPU architecture (aarch64) as the Pi 3 in 64-bit mode, so userland software, recipes, and your own layer's packages behave the same. For a 32-bit Pi 3 userland, use `qemuarm`. Then:

```bash
bitbake core-image-minimal
runqemu qemuarm64 nographic
```

**What you don't get**

QEMU's `qemuarm64` is a generic `virt` board, so none of the Pi's hardware is emulated: no VideoCore, GPIO, the Pi's bootloader and firmware, or the Pi-specific device tree. Anything depending on those must be tested on the real board.

**Emulating the actual Pi 3**

Upstream QEMU has a `raspi3b` machine (`qemu-system-aarch64 -M raspi3b ...`). Its peripheral support is partial, and it needs the Pi kernel, a matching DTB, and a manual command line. I'm not aware of a clean, supported `runqemu` path for it in `meta-raspberrypi`, so check the layer's README on your branch. If you need it, you'd run QEMU by hand against the built kernel and image.

A common workflow is to develop and test general software on `qemuarm64`, and only build `raspberrypi3-64` when you need to verify on the Pi. You can keep two build directories for this and share `DL_DIR` and `SSTATE_DIR` between them.

# When should I bitbake-layers create-layers-setup ?

Run it once your layers are in a state you want to reproduce. It records the layers currently listed in `bblayers.conf` (their git remotes and exact commits) so someone else can recreate the same checkout.

```bash
bitbake-layers create-layers-setup /path/to/output-dir
```

That produces two files:

- `setup-layers.json`: the layer list, each with its remote URL, branch, and pinned revision.
- `setup-layers`: a script that clones and checks out those layers.

A new machine or CI job can then run `./setup-layers` to get all the layers at the recorded revisions. It doesn't create the build directory or configs, so you still run `oe-init-build-env` afterwards.

**Good times to run it**
- After you've added the layers you need (poky, meta-openembedded, meta-raspberrypi, your own) and confirmed a build works.
- Before a release or handoff, so an image can be traced to exact metadata.
- Whenever you update layer revisions and want to refresh the pinned set.

**Caveats**
- Layers must be git checkouts with a remote. A layer with no remote or with uncommitted local changes can't be captured faithfully, and you'll get warnings. Push or commit your own layer first.
- You commit the output to a config repo. Your own `meta-myproject` is normally tracked there as one of the layers.
- It only pins layers, not `local.conf` settings, so keep those in your layer, in a `TEMPLATECONF`, or in the config repo alongside the output.

**When to skip it**

If you're using `kas`, you don't need it: the kas YAML already pins repos and revisions and also sets machine, distro, and `local.conf` options, so it covers the same ground and more. `create-layers-setup` is the choice if you'd rather stay with plain BitBake tooling and not add a dependency.

# What does 'meta' mean?

In Yocto, "meta" is short for **metadata**: the instructions that describe how to build software, as opposed to the software itself.

**Two kinds of content**
- **Source code** is what gets built: the Linux kernel, busybox, your application.
- **Metadata** is everything that tells BitBake how to build it: recipes (`.bb`), bbappends, classes (`.bbclass`), and configuration (`.conf`, `.inc`). It covers where to fetch sources, what patches to apply, what depends on what, and how to configure, compile, and package.

The prefix comes from the Greek "beyond" or "about", the same sense as in "metadata" (data about data). The metadata is information about how to produce software.

**Where you see it**
- **`meta-*` layer names** are a convention meaning "a layer of metadata": `meta-openembedded`, `meta-raspberrypi`, `meta-myproject`. The prefix isn't required, but nearly everyone follows it, and `bitbake-layers create-layer` sets it up that way.
- **`poky/meta`** is the OpenEmbedded-Core layer (often written OE-Core), the foundation with the core recipes and classes.
- **`poky/meta-poky`** holds the Poky distribution policy, and **`poky/meta-yocto-bsp`** holds reference hardware support (BSP is "board support package").
- **`poky/bitbake`** is the build engine, not metadata. It's the tool that parses the metadata and runs the tasks.

So when the primer said "you describe what you want in metadata", that means writing recipes and config files in layers, which BitBake then turns into a bootloader, kernel, and root filesystem.
