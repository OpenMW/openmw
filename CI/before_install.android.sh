#!/bin/sh -ex

curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/android/openmw-android-deps-20211114.zip -o ~/openmw-android-deps.zip
unzip -o ~/openmw-android-deps -d /android-ndk-r22/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr > /dev/null
