#!/bin/sh -e

brew update

brew outdated cmake || brew upgrade cmake
brew outdated pkgconfig || brew upgrade pkgconfig
brew install qt

curl -fSL -R -J https://downloads.openmw.org/osx/dependencies/openmw-deps-7cf2789.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
