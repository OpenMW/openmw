#!/bin/sh

export CXX=clang++
export CC=clang

brew tap openmw/openmw
brew update
brew unlink boost
brew install cmake openmw-mygui openmw-bullet openmw-sdl2 openmw-ffmpeg pkg-config qt unshield
