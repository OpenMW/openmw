#!/bin/sh -e

# workaround python issue on travis
HOMEBREW_NO_AUTO_UPDATE=1 brew uninstall --ignore-dependencies python@3.8 || true
HOMEBREW_NO_AUTO_UPDATE=1 brew uninstall --ignore-dependencies python@3.9 || true

# Some of these tools can come from places other than brew, so check before installing
command -v ccache >/dev/null 2>&1 || brew install ccache
command -v cmake >/dev/null 2>&1 || brew install cmake
command -v qmake >/dev/null 2>&1 || brew install qt

curl -fSL -R -J https://downloads.openmw.org/osx/dependencies/openmw-deps-f8918dd.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
