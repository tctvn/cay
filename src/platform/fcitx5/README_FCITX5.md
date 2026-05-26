# CayIME Fcitx5 Plugin

This directory contains the native Fcitx5 plugin for CayIME. It integrates directly with the Fcitx5 Input Method Framework on Linux, providing a seamless "Direct Input" experience (no preedit underline) using standard `deleteSurroundingText` and `commitString` APIs.

## Prerequisites

You need the Fcitx5 development headers and standard build tools. On Ubuntu/Debian, install them via:

```bash
sudo apt update
sudo apt install -y build-essential cmake extra-cmake-modules fcitx5 libfcitx5-dev fcitx5-modules-dev
```

On Arch Linux:
```bash
sudo pacman -S base-devel cmake extra-cmake-modules fcitx5
```

## Build and Install (System Wide)

You can build and install this plugin to your system directories.

1. Go to the root of the `cay` project.
2. Configure CMake with the `BUILD_FCITX5` option and set the install prefix to `/usr`:

```bash
mkdir -p build-fcitx5
cd build-fcitx5
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_FCITX5=ON
```

3. Build and install:

```bash
make
sudo make install
```

## Usage

1. Restart Fcitx5 so it can discover the new plugin:
```bash
fcitx5 -r &
```

2. Open the Fcitx5 Configuration UI:
```bash
fcitx5-configtool
```

3. Turn off "Only Show Current Language" (if CayIME is not showing up under your locale).
4. Search for "CayIME" (or "Cay") in the Available Input Methods list.
5. Add it to your Current Input Methods.
6. Open a text editor, switch to CayIME, and start typing Vietnamese!
