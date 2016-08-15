#!/bin/sh

export CXX=clang++
export CC=clang

DEPENDENCIES_ROOT="/private/tmp/openmw-deps/openmw-deps"
QT_PATH="/usr/local/opt/qt5"

mkdir build
cd build

cmake \
-D CMAKE_EXE_LINKER_FLAGS="-lz" \
-D CMAKE_PREFIX_PATH="$DEPENDENCIES_ROOT;$QT_PATH" \
-D CMAKE_OSX_DEPLOYMENT_TARGET="10.8" \
-D CMAKE_OSX_SYSROOT="macosx10.11" \
-D CMAKE_BUILD_TYPE=Debug \
-D OPENMW_OSX_DEPLOYMENT=TRUE \
-D DESIRED_QT_VERSION=5 \
-D BUILD_ESMTOOL=FALSE \
-D BUILD_MYGUI_PLUGIN=FALSE \
-G"Unix Makefiles" \
..
