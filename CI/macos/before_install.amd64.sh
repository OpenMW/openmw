#!/bin/sh -ex

arch -x86_64 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

arch -x86_64 /usr/local/bin/brew install curl xquartz gd fontconfig freetype harfbuzz brotli ccache cmake qt@6 openal-soft icu4c yaml-cpp sqlite

curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20240802.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /tmp > /dev/null
