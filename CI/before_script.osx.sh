#!/bin/sh -e

export CXX=clang++
export CC=clang

DEPENDENCIES_ROOT="/private/tmp/openmw-deps/openmw-deps"
QT_PATH=$(brew --prefix qt)
CCACHE_EXECUTABLE=$(brew --prefix ccache)/bin/ccache
mkdir build
cd build

cmake \
-D CMAKE_PREFIX_PATH="$DEPENDENCIES_ROOT;$QT_PATH" \
-D CMAKE_C_COMPILER_LAUNCHER="$CCACHE_EXECUTABLE" \
-D CMAKE_CXX_COMPILER_LAUNCHER="$CCACHE_EXECUTABLE" \
-D CMAKE_OSX_DEPLOYMENT_TARGET="10.9" \
-D CMAKE_OSX_SYSROOT="macosx10.14" \
-D CMAKE_BUILD_TYPE=Release \
-D OPENMW_OSX_DEPLOYMENT=TRUE \
-D DESIRED_QT_VERSION=5 \
-D BUILD_ESMTOOL=FALSE \
-D BUILD_MYGUI_PLUGIN=FALSE \
-G"Unix Makefiles" \
..
