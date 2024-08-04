#!/bin/sh -ex

export HOMEBREW_NO_EMOJI=1
export HOMEBREW_NO_INSTALL_CLEANUP=1
export HOMEBREW_AUTOREMOVE=1

# workaround for gitlab's pre-installed brew
# purge large and unnecessary packages that get in our way and have caused issues
brew uninstall ruby php openjdk node postgresql maven curl || true

brew tap --repair
brew update --quiet

# Some of these tools can come from places other than brew, so check before installing
brew install curl xquartz gd fontconfig freetype harfbuzz brotli

command -v ccache >/dev/null 2>&1 || brew install ccache
command -v cmake >/dev/null 2>&1 || brew install cmake
command -v qmake >/dev/null 2>&1 || brew install qt@5
export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"

# Install deps
brew install openal-soft icu4c yaml-cpp sqlite

ccache --version
cmake --version
qmake --version

if [[ "${MACOS_AMD64}" ]]; then
    curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20221113.zip -o ~/openmw-deps.zip
else
    curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20240802_arm64.zip -o ~/openmw-deps.zip
fi
unzip -o ~/openmw-deps.zip -d /tmp > /dev/null
