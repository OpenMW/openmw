#!/bin/sh -e

# Some of these tools can come from places other than brew, so check before installing
echo line 4
command -v ccache >/dev/null 2>&1 || brew install ccache
echo line 6
command -v cmake >/dev/null 2>&1 || brew install cmake
echo line 8
command -v qmake >/dev/null 2>&1 || brew install qt
echo line 10
curl -fSL -R -J https://downloads.openmw.org/osx/dependencies/openmw-deps-f8918dd.zip -o ~/openmw-deps.zip
echo line 12
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
echo line 14
