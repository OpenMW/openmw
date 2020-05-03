#!/bin/sh -e

git clone -b release-1.10.0 https://github.com/google/googletest.git
cd googletest
mkdir build
cd build
cmake \
    -D CMAKE_BUILD_TYPE="${CONFIGURATION}" \
    -D CMAKE_INSTALL_PREFIX=. \
    -G "${GENERATOR}" \
    ..
cmake --build . --config "${CONFIGURATION}"
cmake --build . --target install --config "${CONFIGURATION}"
