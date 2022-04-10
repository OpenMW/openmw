#!/bin/sh -ex

# hack to work around: FFmpeg version is too old, 3.2 is required
sed -i s/"NOT FFVER_OK"/"FALSE"/ CMakeLists.txt

mkdir -p build
cd build

# Build a version of ICU for the host so that it can use the tools during the cross-compilation
mkdir -p icu-host-build
cd icu-host-build
if [ -r ../extern/fetched/icu/icu4c/source/configure ]; then
    ICU_SOURCE_DIR=../extern/fetched/icu/icu4c/source
else
    wget https://github.com/unicode-org/icu/archive/refs/tags/release-70-1.zip
    unzip release-70-1.zip
    ICU_SOURCE_DIR=./icu-release-70-1/icu4c/source
fi
${ICU_SOURCE_DIR}/configure --disable-tests --disable-samples --disable-icuio --disable-extras CC="ccache gcc" CXX="ccache g++"
make -j $(nproc)
cd ..

cmake \
-DCMAKE_TOOLCHAIN_FILE=/android-ndk-r22/build/cmake/android.toolchain.cmake \
-DANDROID_ABI=arm64-v8a \
-DANDROID_PLATFORM=android-21 \
-DANDROID_LD=deprecated \
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
-DBUILD_NAVMESHTOOL=OFF \
-DBUILD_BULLETOBJECTTOOL=OFF \
-DOPENMW_USE_SYSTEM_MYGUI=OFF \
-DOPENMW_USE_SYSTEM_SQLITE3=OFF \
-DOPENMW_USE_SYSTEM_YAML_CPP=OFF \
-DOPENMW_USE_SYSTEM_ICU=OFF \
-DOPENMW_ICU_HOST_BUILD_DIR="$(pwd)/icu-host-build" \
..
