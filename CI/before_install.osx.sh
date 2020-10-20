#!/bin/sh -e

# Some of these tools can come from places other than brew, so check before installing
command -v ccache >/dev/null 2>&1 || brew install ccache
command -v cmake >/dev/null 2>&1 || brew install cmake
command -v qmake >/dev/null 2>&1 || brew install qt

brew link --overwrite lz4  #  overwrite system lz4; use brew
brew reinstall lz4

curl -fSL -R -J https://downloads.openmw.org/osx/dependencies/openmw-deps-ef2462c.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
