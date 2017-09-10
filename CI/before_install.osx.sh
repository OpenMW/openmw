#!/bin/sh

brew update

brew outdated cmake || brew upgrade cmake
brew outdated pkgconfig || brew upgrade pkgconfig
brew install $macos_qt_formula

curl https://downloads.openmw.org/osx/dependencies/openmw-deps-5e144e2.zip -o ~/openmw-deps.zip
unzip ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
