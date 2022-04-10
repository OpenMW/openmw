#!/bin/sh -ex

export HOMEBREW_NO_EMOJI=1
brew update --quiet

# workaround python issue on travis
[ -z "${TRAVIS}" ] && brew uninstall --ignore-dependencies python@3.8 || true
[ -z "${TRAVIS}" ] && brew uninstall --ignore-dependencies python@3.9 || true
[ -z "${TRAVIS}" ] && brew uninstall --ignore-dependencies qt@6 || true

# Some of these tools can come from places other than brew, so check before installing
[ -z "${TRAVIS}" ] && brew reinstall fontconfig
command -v ccache >/dev/null 2>&1 || brew install ccache
command -v cmake >/dev/null 2>&1 || brew install cmake
command -v qmake >/dev/null 2>&1 || brew install qt@5
brew install icu4c
brew install yaml-cpp
export PATH="/usr/local/opt/qt@5/bin:$PATH"  # needed to use qmake in none default path as qt now points to qt6

ccache --version
cmake --version
qmake --version

curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20210716.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null

