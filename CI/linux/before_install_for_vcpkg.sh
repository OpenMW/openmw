#!/bin/bash -ex

. ./CI/linux/deps_versions.sh

pipx install aqtinstall && pipx ensurepath

mkdir -p deps
cd deps

if [[ ! -d ./Qt/$QT_VER ]]; then
    ~/.local/bin/aqt install-qt linux desktop $QT_VER gcc_64 -m all -O ./Qt
fi

VCPKG_DIRNAME=${VCPKG_FILE}-${VCPKG_TAG}
MANIFEST=$VCPKG_DIRNAME-manifest.txt
if [[ ! -f $MANIFEST ]]; then
    curl "https://gitlab.com/AnyOldName3/openmw-deps/-/raw/linux-vcpkg-testing/linux/${MANIFEST}?ref_type=heads" -o ${MANIFEST}
fi

{ read -r URL && read -r HASH FILE; } < $MANIFEST

if [[ ! -d $VCPKG_DIRNAME ]]; then
    if [[ ! -f $FILE ]]; then
        curl -fSL -R -J $URL -o $FILE
    fi
    echo "${HASH:?}  ${FILE:?}" | sha512sum -c
    tar -xf $FILE
    mv vcpkg-export $VCPKG_DIRNAME
fi
