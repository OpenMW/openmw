#!/bin/sh -ex

brew tap --repair
brew update --quiet

brew install curl xquartz gd fontconfig freetype harfbuzz brotli s3cmd ccache cmake qt@6 openal-soft icu4c yaml-cpp sqlite

curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20240818-arm64.tar.xz -o ~/openmw-deps.tar.xz
tar xf ~/openmw-deps.tar.xz -C /tmp > /dev/null
