#!/bin/sh -e

# Some of these tools can come from places other than brew, so check before installing
command -v ccache >/dev/null 2>&1 || brew install ccache
command -v cmake >/dev/null 2>&1 || brew install cmake
command -v qmake >/dev/null 2>&1 || brew install qt

curl -fSL -R -J https://www.dropbox.com/s/86ei812d3knuuki/openmw-deps-f8918dd.zip?dl=1 -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
