#!/bin/sh -e

brew update
brew unlink cmake || true
brew install https://gist.githubusercontent.com/nikolaykasyanov/f36da224bdef42025e480f99fa21a82d/raw/7dd8b5ed2750198757f81c6bc6456e03541999bd/cmake.rb
brew switch cmake 3.12.4
brew outdated pkgconfig || brew upgrade pkgconfig
brew install qt

curl -fSL -R -J https://downloads.openmw.org/osx/dependencies/openmw-deps-110f3d3.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
