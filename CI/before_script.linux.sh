#!/bin/bash -ex

free -m

if [[ "${BUILD_TESTS_ONLY}" ]]; then
    export GOOGLETEST_DIR="$(pwd)/googletest/build/install"
    env GENERATOR='Unix Makefiles' CONFIGURATION=Release CI/build_googletest.sh
fi

mkdir build
cd build

if [[ "${BUILD_TESTS_ONLY}" ]]; then
    ${ANALYZE} cmake \
        -D CMAKE_C_COMPILER="${CC}" \
        -D CMAKE_CXX_COMPILER="${CXX}" \
        -D CMAKE_C_COMPILER_LAUNCHER=ccache \
        -D CMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -D CMAKE_INSTALL_PREFIX=install \
        -D CMAKE_BUILD_TYPE=RelWithDebInfo \
        -D USE_SYSTEM_TINYXML=TRUE \
        -D BUILD_OPENMW=OFF \
        -D BUILD_BSATOOL=OFF \
        -D BUILD_ESMTOOL=OFF \
        -D BUILD_LAUNCHER=OFF \
        -D BUILD_MWINIIMPORTER=OFF \
        -D BUILD_ESSIMPORTER=OFF \
        -D BUILD_OPENCS=OFF \
        -D BUILD_WIZARD=OFF \
        -D BUILD_UNITTESTS=ON \
        -D GTEST_ROOT="${GOOGLETEST_DIR}" \
        -D GMOCK_ROOT="${GOOGLETEST_DIR}" \
        ..
else
    ${ANALYZE} cmake \
        -D CMAKE_C_COMPILER="${CC}" \
        -D CMAKE_CXX_COMPILER="${CXX}" \
        -D CMAKE_C_COMPILER_LAUNCHER=ccache \
        -D CMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -D USE_SYSTEM_TINYXML=TRUE \
        -D CMAKE_INSTALL_PREFIX=install \
        -D CMAKE_BUILD_TYPE=Debug \
        ..
fi
