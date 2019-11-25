#!/bin/sh -e

brew update
brew outdated pkgconfig || brew upgrade pkgconfig
brew unlink cmake || brew install cmake@3.14.2
brew install qt
brew install ccache

curl -fSL -R -J https://downloads.openmw.org/osx/dependencies/openmw-deps-110f3d3.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
