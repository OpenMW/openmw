#!/bin/sh -ex

command -v /usr/local/bin/brew || arch -x86_64 bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

arch -x86_64 bash -c "command -v qmake >/dev/null 2>&1 && qmake -v | grep -F 'Using Qt version 6.' >/dev/null || /usr/local/bin/brew install qt@6"

arch -x86_64 /usr/local/bin/brew install curl xquartz gd fontconfig freetype harfbuzz brotli openal-soft icu4c yaml-cpp sqlite

curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20240802.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /tmp > /dev/null
