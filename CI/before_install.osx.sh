#!/bin/sh -ex

export HOMEBREW_NO_EMOJI=1
export HOMEBREW_NO_INSTALL_CLEANUP=1
export HOMEBREW_AUTOREMOVE=1

brew tap --repair
brew update --quiet

brew install curl xquartz gd fontconfig freetype harfbuzz brotli s3cmd

command -v ccache >/dev/null 2>&1 || brew install ccache
command -v cmake >/dev/null 2>&1 || brew install cmake
command -v qmake >/dev/null 2>&1 || brew install qt@6
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"

# Install deps
brew install openal-soft icu4c yaml-cpp sqlite

ccache --version
cmake --version
qmake --version

if [[ "${MACOS_AMD64}" ]]; then
    curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20240802.zip -o ~/openmw-deps.zip
    unzip -o ~/openmw-deps.zip -d /tmp > /dev/null
else
    curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20240818-arm64.tar.xz -o ~/openmw-deps.tar.xz
    tar xf ~/openmw-deps.tar.xz -C /tmp > /dev/null
fi
