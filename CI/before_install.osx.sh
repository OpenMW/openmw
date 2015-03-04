#!/bin/sh

export CXX=clang++
export CC=clang

brew tap openmw/openmw
brew update
brew unlink boost
brew install openmw-mygui openmw-bullet openmw-sdl2 openmw-ffmpeg openmw/openmw/qt unshield
