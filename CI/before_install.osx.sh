#!/bin/sh -e

brew update
brew outdated pkgconfig || brew upgrade pkgconfig
brew install qt
brew install ccache

# fall back to and older cmake that likes new libboost; this will break once we upgrade to MacOS 10.15
brew unlink cmake
curl -fSL -R -J https://mirrors.aliyun.com/homebrew/homebrew-bottles/bottles/cmake-3.14.6.mojave.bottle.tar.gz -o ~/cmake-3.14.6.mojave.bottle.tar.gz
brew install -f ~/cmake-3.14.6.mojave.bottle.tar.gz

curl -fSL -R -J https://downloads.openmw.org/osx/dependencies/openmw-deps-110f3d3.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
