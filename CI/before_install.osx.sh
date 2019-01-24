#!/bin/sh -e

brew update
brew unlink cmake || true
brew install https://raw.githubusercontent.com/Homebrew/homebrew-core/a3b64391ebace30b84de8e7997665a1621c0b2c0/Formula/cmake.rb
brew switch cmake 3.12.4
brew outdated pkgconfig || brew upgrade pkgconfig
brew install qt

curl -fSL -R -J https://downloads.openmw.org/osx/dependencies/openmw-deps-110f3d3.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
