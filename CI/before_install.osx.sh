#!/bin/sh

brew update

brew rm cmake || true
brew rm pkgconfig || true
brew rm qt5 || true
brew install cmake pkgconfig $macos_qt_formula

curl https://downloads.openmw.org/osx/dependencies/openmw-deps-0ecece4.zip -o ~/openmw-deps.zip
unzip ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
