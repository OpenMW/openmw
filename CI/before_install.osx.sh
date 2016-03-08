#!/bin/sh

brew update
brew rm cmake || true
brew rm pkgconfig || true
brew rm qt5 || true
brew install cmake pkgconfig qt5

curl http://downloads.openmw.org/osx/dependencies/openmw-deps-263d4a8.zip -o ~/openmw-deps.zip
unzip ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
