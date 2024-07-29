#!/bin/sh -e

export CXX=clang++
export CC=clang

# Silence a git warning
git config --global advice.detachedHead false

mkdir build
cd build

DEPENDENCIES_ROOT="/tmp/openmw-deps"

QT_PATH=$(brew --prefix qt@5)
ICU_PATH=$(brew --prefix icu4c)
OPENAL_PATH=$(brew --prefix openal-soft)
CCACHE_EXECUTABLE=$(brew --prefix ccache)/bin/ccache

declare -a CMAKE_CONF_OPTS=(
-D CMAKE_PREFIX_PATH="$DEPENDENCIES_ROOT;$QT_PATH;$OPENAL_PATH"
-D CMAKE_C_COMPILER_LAUNCHER="$CCACHE_EXECUTABLE"
-D CMAKE_CXX_COMPILER_LAUNCHER="$CCACHE_EXECUTABLE"
-D CMAKE_CXX_FLAGS="-stdlib=libc++"
-D CMAKE_OSX_DEPLOYMENT_TARGET="13.6"
-D OPENMW_USE_SYSTEM_OSG=TRUE
-D OPENMW_USE_SYSTEM_RECASTNAVIGATION=TRUE
-D Boost_INCLUDE_DIR="$DEPENDENCIES_ROOT/include"
-D OSGPlugins_LIB_DIR="$DEPENDENCIES_ROOT/lib/osgPlugins-3.6.5"
-D ICU_ROOT="$ICU_PATH"
-D OPENMW_OSX_DEPLOYMENT=TRUE
)

if [[ "${CMAKE_BUILD_TYPE}" ]]; then
    CMAKE_CONF_OPTS+=(
        -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    )
else
    CMAKE_CONF_OPTS+=(
        -D CMAKE_BUILD_TYPE=RelWithDebInfo
    )
fi

if [[ "${FOR_RELEASE}" ]]; then
    CMAKE_CONF_OPTS+=(
        -D CMAKE_C_FLAGS_RELEASE="-g -O0"
        -D CMAKE_CXX_FLAGS_RELEASE="-g -O0"
    )
fi

cmake \
"${CMAKE_CONF_OPTS[@]}" \
-D BUILD_OPENMW=TRUE \
-D BUILD_OPENCS=TRUE \
-D BUILD_ESMTOOL=TRUE \
-D BUILD_BSATOOL=TRUE \
-D BUILD_ESSIMPORTER=TRUE \
-D BUILD_NIFTEST=TRUE \
-D BUILD_NAVMESHTOOL=TRUE \
-D BUILD_BULLETOBJECTTOOL=TRUE \
-G"Unix Makefiles" \
..
