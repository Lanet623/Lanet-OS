# Lanet Shell

A first implementation of Lanet OS's custom graphical shell. The shell is a GTK3 application designed to run on Sway/Wayland, but it remains usable in an X11 session for development.

## Current features

- Full-screen custom desktop
- Application grid loaded from `.desktop` files
- Searchable launcher
- Clock in the top panel
- Dock with file manager, terminal, settings, and session exit
- Lanet Control Center for user appearance, APK packages, and OpenRC services
- Lanet CSS theme and hover/press transitions

## Alpine dependencies

Build dependencies:

```sh
doas apk add alpine-sdk cmake gtk+3.0-dev
```

Runtime dependencies:

```sh
doas apk add gtk+3.0 sway xfce4-terminal thunar doas
```

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
doas cmake --install build
```

For a local test without installing:

```sh
cmake -S . -B build
cmake --build build
./build/lanet-shell
```

## Run under Sway

Copy `data/sway-lanet.conf` to the end of `~/.config/sway/config`, or start Sway manually first and execute:

```sh
lanet-shell
```

This first version is a shell UI over Sway, not a replacement wlroots compositor. Later versions can replace the separate shell window with native wlroots surfaces and Wayland protocols.

Control Center stores the appearance preference in `~/.config/lanet/registry.ini`. Package install/removal and OpenRC service changes are launched in a terminal through `doas`; package/service identifiers are validated and services are restricted to an allowlist.
