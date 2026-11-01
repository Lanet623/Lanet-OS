# Lanet OS

Lanet OS is a customizable Alpine Linux project. Its first build targets **Alpine Linux 3.24, x86_64** and uses Alpine's official `aports/scripts/mkimage.sh` builder. The image is intentionally minimal; it is not a new kernel or a complete desktop distribution yet.

## Why Alpine

Alpine uses musl, BusyBox, and OpenRC and has a small base install. This makes it a reasonable starting point for older PCs, but it does not guarantee that every desktop workload will run well. A graphical desktop and browser need more memory than the base system.

The first custom ISO is a lightweight Alpine diskless/live image. It includes the Lanet Shell and Lanet Control Center binaries; a first-boot OpenRC service installs their GTK/Sway runtime packages from the APK repository on the ISO. Alpine diskless mode loads the system into RAM, so this graphical image still needs more memory than a command-line installation.

## Build host

The supported build host is **Alpine Linux**, preferably in a VirtualBox VM on Windows. The official Alpine image builder depends on `apk`, `abuild`, and Alpine-specific tooling; native Windows, Ubuntu, and Debian are not supported hosts for this script.

Use the steps in [QUICKSTART.md](QUICKSTART.md). A stopped VirtualBox VM named **Lanet OS Builder Alpine** has already been prepared with 1536 MB RAM, 2 CPUs, and a 20 GB virtual disk. The official Alpine 3.24.3 ISO and its `.sha256` file are in `output/`. Plan for internet and at least 20 GB free disk space. The first build can take roughly 30-90 minutes, depending on network and hardware.

## Build on Alpine

After Alpine is installed and networking is working, copy or share the project directory into the Alpine VM. From the project directory on that Alpine host:

```sh
doas sh ./build-lanetos.sh
```

The script installs build dependencies, creates a signing key for an unprivileged package builder, downloads the official `aports` scripts, builds the Lanet APK, passes its signed local repository to the custom `lanetos` profile, checks the ISO boot records, and copies it to `output/lanetos-alpine-3.24-x86_64.iso`.

## First boot

The next ISO build is configured to start the `lanet` user session automatically through greetd, Sway, and Lanet Shell. A boot-time service installs the signed Lanet APK and runtime packages from the ISO repository, configures eudev/input devices, and enables the session services. These startup changes are in source only until a new ISO is built; older ISO files will still boot to the previous behavior.

Test the new ISO in a separate VM first. Installation to a physical disk, persistence, and hardware compatibility are not validated. `setup-alpine` in system-disk mode erases the selected disk.

The image has not yet been tested on physical hardware. Secure Boot, persistence, automatic graphical login, custom artwork, and a branded installer remain unverified until the updated ISO is built and tested.

## Custom graphical shell

The `lanet-shell/` project contains Lanet OS's graphical interface and Control Center. Both are written in C/GTK3 and run over Sway/Wayland. The shell includes a launcher, application search, clock, dock, custom CSS theme, and Sway integration. Control Center stores user appearance settings in `~/.config/lanet/registry.ini`, and manages APK packages and OpenRC boot services through a `doas` terminal prompt.

This version is an independent shell UI over Sway, not yet a custom wlroots compositor. It can be redesigned and tested without replacing Linux window management itself.

Build both applications inside Alpine with:

```sh
cd lanet-shell
doas sh ./build-alpine.sh
lanet-shell
```

## Customize

Edit `mkimg.lanetos.sh` or the files under `lanet-shell/` to change the image profile or graphical interface. Keep the official base profile and boot setup unless you know which Alpine mkimage section you are replacing. Rebuild and test in a VM after each change.