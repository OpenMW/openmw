#!/bin/sh -e

# Silence a git warning
git config --global advice.detachedHead false

rm -fr build
mkdir build
cd build

DEPENDENCIES_ROOT="/tmp/openmw-deps"

if [[ "${MACOS_X86_64}" ]]; then
    QT_PATH=$(arch -x86_64 brew --prefix qt@6)
    ICU_PATH=$(arch -x86_64 brew --prefix icu4c)
    OPENAL_PATH=$(arch -x86_64 brew --prefix openal-soft)
    CCACHE_EXECUTABLE=$(arch -x86_64 brew --prefix ccache)/bin/ccache
else
    QT_PATH=$(brew --prefix qt@6)
    ICU_PATH=$(brew --prefix icu4c)
    OPENAL_PATH=$(brew --prefix openal-soft)
    CCACHE_EXECUTABLE=$(brew --prefix ccache)/bin/ccache
fi

declare -a CMAKE_CONF_OPTS=(
-D CMAKE_PREFIX_PATH="$DEPENDENCIES_ROOT;$QT_PATH;$OPENAL_PATH"
-D CMAKE_C_COMPILER_LAUNCHER="$CCACHE_EXECUTABLE"
-D CMAKE_CXX_COMPILER_LAUNCHER="$CCACHE_EXECUTABLE"
-D CMAKE_CXX_FLAGS="-stdlib=libc++"
-D CMAKE_C_COMPILER="clang"
-D CMAKE_CXX_COMPILER="clang++"
-D CMAKE_OSX_DEPLOYMENT_TARGET="13.6"
-D OPENMW_USE_SYSTEM_RECASTNAVIGATION=TRUE
-D Boost_INCLUDE_DIR="$DEPENDENCIES_ROOT/include"
-D OSGPlugins_LIB_DIR="$DEPENDENCIES_ROOT/lib/osgPlugins-3.6.5"
-D ICU_ROOT="$ICU_PATH"
-D OPENMW_OSX_DEPLOYMENT=TRUE
)

declare -a BUILD_OPTS=(
-D BUILD_OPENMW=TRUE
-D BUILD_OPENCS=TRUE
-D BUILD_ESMTOOL=TRUE
-D BUILD_BSATOOL=TRUE
-D BUILD_ESSIMPORTER=TRU
-D BUILD_NIFTEST=TRUE
-D BUILD_NAVMESHTOOL=TRUE
-D BUILD_BULLETOBJECTTOOL=TRUE
-G"Unix Makefiles"
)

if [[ "${MACOS_X86_64}" ]]; then
    CMAKE_CONF_OPTS+=(
        -D CMAKE_OSX_ARCHITECTURES="x86_64"
    )
fi

if [[ "${CMAKE_BUILD_TYPE}" ]]; then
    CMAKE_CONF_OPTS+=(
        -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    )
else
    CMAKE_CONF_OPTS+=(
        -D CMAKE_BUILD_TYPE=RelWithDebInfo
    )
fi

if [[ "${MACOS_X86_64}" ]]; then
    arch -x86_64 cmake \
        "${CMAKE_CONF_OPTS[@]}" \
        "${BUILD_OPTS[@]}" \
        ..
else
    cmake \
        "${CMAKE_CONF_OPTS[@]}" \
        "${BUILD_OPTS[@]}" \
        ..
fi
