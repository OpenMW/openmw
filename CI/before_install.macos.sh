#!/bin/sh -ex

if [[ "${MACOS_AMD64}" ]]; then
    ./CI/macos/before_install.amd64.sh
else
    ./CI/macos/before_install.arm64.sh
fi

command -v ccache >/dev/null 2>&1 || brew install ccache
command -v cmake >/dev/null 2>&1 || brew install cmake
