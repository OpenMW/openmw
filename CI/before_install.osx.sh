#!/bin/sh -e

brew update
brew outdated pkgconfig || brew upgrade pkgconfig
brew install qt
brew install ccache

# get latest llvm/clang
brew install llvm # llvm but with all the headers
sudo rm -rf /Library/Developer/CommandLineTools
xcode-select --install # installs additional headers that you might be missing.
echo 'export PATH="/usr/local/opt/llvm/bin:$PATH"' >> ~/.bash_profile # exports the custom llvm path into the shell
sudo ln -s /usr/local/opt/llvm/bin/clang++ /usr/local/bin/clang++-brew # optional but I like to have a symlink set.

curl -fSL -R -J https://downloads.openmw.org/osx/dependencies/openmw-deps-110f3d3.zip -o ~/openmw-deps.zip
unzip -o ~/openmw-deps.zip -d /private/tmp/openmw-deps > /dev/null
