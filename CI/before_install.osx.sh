#!/bin/sh -e

brew update
brew outdated pkgconfig || brew upgrade pkgconfig
brew install qt
brew install ccache

# fall back to and older cmake that likes new libboost
brew unlink cmake
cd "$(brew --repo homebrew/core)" && git log master -- Formula/cmake.rb
cd "$(brew --repo homebrew/core)" && git checkout 74d54460d0af8e21deacee8dcced325cfc191141  # 3.15.4
HOMEBREW_NO_AUTO_UPDATE=1 brew install cmake

curl -fSL -R -J https://downloads.openmw.org/osx/dependencies/openmw-deps-110f3d3.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
