#!/bin/sh -ex

# workaround python issue on travis
[ -z "${TRAVIS}" ] && HOMEBREW_NO_AUTO_UPDATE=1 brew uninstall --ignore-dependencies python@3.8 || true
[ -z "${TRAVIS}" ] && HOMEBREW_NO_AUTO_UPDATE=1 brew uninstall --ignore-dependencies python@3.9 || true
[ -z "${TRAVIS}" ] && HOMEBREW_NO_AUTO_UPDATE=1 brew uninstall --ignore-dependencies qt@6 || true

# Some of these tools can come from places other than brew, so check before installing
command -v ccache >/dev/null 2>&1 || brew install ccache
command -v cmake >/dev/null 2>&1 || brew install cmake
command -v qmake >/dev/null 2>&1 || brew install qt@5
export PATH="/usr/local/opt/qt@5/bin:$PATH"  # needed to use qmake in none default path as qt now points to qt6

ccache --version
cmake --version
qmake --version

curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20210617.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null

# additional libraries
[ -z "${TRAVIS}" ] && HOMEBREW_NO_AUTO_UPDATE=1 brew install fontconfig