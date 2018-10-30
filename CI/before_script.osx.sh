#!/bin/sh -e

export CXX=clang++
export CC=clang

env GENERATOR='Unix Makefiles' CONFIGURATION=Release CI/build_recastnavigation.sh
RECASTNAVIGATION_DIR="$(pwd)/recastnavigation/build"

DEPENDENCIES_ROOT="/private/tmp/openmw-deps/openmw-deps"
QT_PATH=`brew --prefix qt`
mkdir build
cd build

cmake \
-D CMAKE_PREFIX_PATH="$DEPENDENCIES_ROOT;$QT_PATH" \
-D CMAKE_OSX_DEPLOYMENT_TARGET="10.9" \
-D CMAKE_OSX_SYSROOT="macosx10.13" \
-D CMAKE_BUILD_TYPE=Release \
-D OPENMW_OSX_DEPLOYMENT=TRUE \
-D DESIRED_QT_VERSION=5 \
-D BUILD_ESMTOOL=FALSE \
-D BUILD_MYGUI_PLUGIN=FALSE \
-D RecastNavigation_ROOT="${RECASTNAVIGATION_DIR}" \
-G"Unix Makefiles" \
..
