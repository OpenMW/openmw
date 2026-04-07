#!/bin/bash -ex

. ./CI/linux/deps_versions.sh

pipx install aqtinstall && pipx ensurepath

~/.local/bin/aqt install-qt linux desktop $QT_VER gcc_64 -m all -O /tmp/QT

curl "https://gitlab.com/AnyOldName3/openmw-deps/-/raw/linux-vcpkg-testing/linux/${VCPKG_FILE}-${VCPKG_TAG}-manifest.txt?ref_type=heads" -o openmw-manifest.txt

{ read -r URL && read -r HASH FILE; } < openmw-manifest.txt

curl -fSL -R -J $URL -o $FILE
echo "${HASH:?}  ${FILE:?}" | sha512sum -c
tar -xf $FILE
mv vcpkg-export $VCPKG_DIRNAME
