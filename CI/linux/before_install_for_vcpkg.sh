#!/bin/bash -ex

. ./CI/linux/deps_versions.sh

pipx install aqtinstall

mkdir -p deps
cd deps

if [[ ! -d ./Qt/$QT_VER ]]; then
    ~/.local/bin/aqt install-qt linux desktop $QT_VER gcc_64 -m all -O ./Qt
fi

VCPKG_TRIPLET='x64-linux-dynamic'
VCPKG_PATH=vcpkg-${VCPKG_TRIPLET}-${VCPKG_TAG}
MANIFEST=$VCPKG_PATH-manifest.txt
if [[ ! -f $MANIFEST ]]; then
    curl "https://gitlab.com/OpenMW/openmw-deps/-/raw/main/linux/${MANIFEST}?ref_type=heads" -o ${MANIFEST}
fi

{ read -r URL && read -r HASH FILE; } < $MANIFEST

if [[ ! -d $VCPKG_PATH ]]; then
    if [[ ! -f $FILE ]]; then
        curl -fSL -R -J $URL -o $FILE
    fi
    echo "${HASH:?}  ${FILE:?}" | sha512sum -c
    tar -xf $FILE
    mv vcpkg-export $VCPKG_PATH
fi
