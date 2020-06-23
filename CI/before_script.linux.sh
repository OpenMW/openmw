#!/bin/bash -ex

free -m

env GENERATOR='Unix Makefiles' CONFIGURATION=Release CI/build_googletest.sh
GOOGLETEST_DIR="$(pwd)/googletest/build"

mkdir build
cd build

${ANALYZE} cmake \
    -DCMAKE_C_COMPILER="${CC}" \
    -DCMAKE_CXX_COMPILER="${CXX}" \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DBUILD_UNITTESTS=TRUE \
    -DUSE_SYSTEM_TINYXML=TRUE \
    -DCMAKE_INSTALL_PREFIX="/usr" \
    -DBINDIR="/usr/games" \
    -DCMAKE_BUILD_TYPE="DEBUG" \
    -DGTEST_ROOT="${GOOGLETEST_DIR}" \
    -DGMOCK_ROOT="${GOOGLETEST_DIR}" \
    ..
