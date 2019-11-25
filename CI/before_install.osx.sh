#!/bin/sh -e

brew update
brew outdated pkgconfig || brew upgrade pkgconfig
brew install qt
brew install ccache

# fall back to and older cmake that likes new libboost
brew unlink cmake
cd "$(brew --repo homebrew/core)" && git checkout 787d9349e5bf9b2791d17e47e3f93aadb3df3ad0  # 3.15.4
HOMEBREW_NO_AUTO_UPDATE=1 brew install cmake

curl -fSL -R -J https://downloads.openmw.org/osx/dependencies/openmw-deps-110f3d3.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
