#!/bin/sh -ex

curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/android/openmw-android-deps-20201129.zip -o ~/openmw-android-deps.zip
unzip -o ~/openmw-android-deps -d /usr/lib/android-sdk/ndk-bundle/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr > /dev/null
