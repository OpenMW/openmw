# Building for the Switch

You need latest versions of devkitA64 and libnx installed.

## Building dependencies

See [this repo](https://github.com/fgsfdsfgs/openmw-switch-deps).

The rest of the dependencies can be acquired using `dkp-pacman`.

## Building OpenMW

```
source $DEVKITPRO/switchvars.sh
mkdir switchbuild && cd switchbuild
cmake \
-G"Unix Makefiles" \
-DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/switch.cmake" \
-DCMAKE_BUILD_TYPE=Release \
-DPKG_CONFIG_EXECUTABLE="$DEVKITPRO/portlibs/switch/bin/aarch64-none-elf-pkg-config" \
-DCMAKE_INSTALL_PREFIX="$DEVKITPRO/portlibs/switch" \
-DMyGUI_LIBRARY="$DEVKITPRO/portlibs/switch/lib/libMyGUIEngineStatic.a" \
-DBUILD_BSATOOL=OFF \
-DBUILD_NIFTEST=OFF \
-DBUILD_ESMTOOL=OFF \
-DBUILD_LAUNCHER=OFF \
-DBUILD_MWINIIMPORTER=OFF \
-DBUILD_ESSIMPORTER=OFF \
-DBUILD_OPENCS=OFF \
-DBUILD_WIZARD=OFF \
-DBUILD_MYGUI_PLUGIN=OFF \
-DOSG_STATIC=TRUE \
..
```
