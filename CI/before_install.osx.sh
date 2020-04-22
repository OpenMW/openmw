#!/bin/sh -e

brew update
brew outdated pkgconfig || brew upgrade pkgconfig
brew install qt
brew install ccache

curl -fSL -R -J https://downloads.openmw.org/osx/dependencies/openmw-deps-ef2462c.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
