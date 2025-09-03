#!/bin/sh -ex

brew tap --repair
brew update --quiet

command -v cmake >/dev/null 2>&1 || brew install cmake

brew install curl xquartz gd fontconfig freetype harfbuzz brotli qt@6 ccache openal-soft icu4c yaml-cpp sqlite

curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20240818-arm64.tar.xz -o ~/openmw-deps.tar.xz
tar xf ~/openmw-deps.tar.xz -C /tmp > /dev/null
