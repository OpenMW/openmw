#!/bin/sh -ex

# hack to work around: FFmpeg version is too old, 3.2 is required
sed -i s/"NOT FFVER_OK"/"FALSE"/ CMakeLists.txt

mkdir build
cd build

cmake \
-DCMAKE_TOOLCHAIN_FILE=/usr/lib/android-sdk/ndk-bundle/build/cmake/android.toolchain.cmake \
-DANDROID_ABI=arm64-v8a \
-DANDROID_PLATFORM=android-21 \
-DCMAKE_C_COMPILER_LAUNCHER=ccache \
-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
-DCMAKE_INSTALL_PREFIX=install \
-DBUILD_BSATOOL=0 \
-DBUILD_NIFTEST=0 \
-DBUILD_ESMTOOL=0 \
-DBUILD_LAUNCHER=0 \
-DBUILD_MWINIIMPORTER=0 \
-DBUILD_ESSIMPORTER=0 \
-DBUILD_OPENCS=0 \
-DBUILD_WIZARD=0 \
-DMyGUI_LIBRARY="/usr/lib/android-sdk/ndk-bundle/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/libMyGUIEngineStatic.a" \
..
