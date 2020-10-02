#!/bin/sh -ex

git clone -b release-1.10.0 https://github.com/google/googletest.git
cd googletest
mkdir build
cd build
cmake \
    -D CMAKE_C_COMPILER="${CC}" \
    -D CMAKE_CXX_COMPILER="${CXX}" \
    -D CMAKE_C_COMPILER_LAUNCHER=ccache \
    -D CMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -D CMAKE_BUILD_TYPE="${CONFIGURATION}" \
    -D CMAKE_INSTALL_PREFIX="${GOOGLETEST_DIR}" \
    -G "${GENERATOR}" \
    ..
cmake --build . --config "${CONFIGURATION}" -- -j $(nproc)
cmake --install . --config "${CONFIGURATION}"
