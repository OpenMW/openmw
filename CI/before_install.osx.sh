#!/bin/sh -ex

export HOMEBREW_NO_EMOJI=1
export HOMEBREW_NO_INSTALL_CLEANUP=1

brew uninstall --ignore-dependencies jpeg || true

brew tap --repair
brew update --quiet

# Some of these tools can come from places other than brew, so check before installing
brew reinstall xquartz fontconfig freetype harfbuzz brotli

# Fix: can't open file: @loader_path/libbrotlicommon.1.dylib (No such file or directory)
BREW_LIB_PATH="$(brew --prefix)/lib"
install_name_tool -change "@loader_path/libbrotlicommon.1.dylib" "${BREW_LIB_PATH}/libbrotlicommon.1.dylib" ${BREW_LIB_PATH}/libbrotlidec.1.dylib
install_name_tool -change "@loader_path/libbrotlicommon.1.dylib" "${BREW_LIB_PATH}/libbrotlicommon.1.dylib" ${BREW_LIB_PATH}/libbrotlienc.1.dylib

command -v ccache >/dev/null 2>&1 || brew install ccache
command -v cmake >/dev/null 2>&1 || brew install cmake
command -v qmake >/dev/null 2>&1 || brew install qt@6
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"


# Install deps
brew install icu4c yaml-cpp sqlite

ccache --version
cmake --version
qmake --version

if [[ "${MACOS_AMD64}" ]]; then
    curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20221113.zip -o ~/openmw-deps.zip
else
    curl -fSL -R -J https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/openmw-deps-20231022_arm64.zip -o ~/openmw-deps.zip
fi

unzip -o ~/openmw-deps.zip -d /tmp > /dev/null

