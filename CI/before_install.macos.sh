#!/bin/sh -ex

brew tap --repair
brew update --quiet

if [[ "${MACOS_AMD64}" ]]; then
    ./CI/macos/before_install.amd64.sh
else
    ./CI/macos/before_install.arm64.sh
fi

command -v cmake >/dev/null 2>&1 || brew install cmake
