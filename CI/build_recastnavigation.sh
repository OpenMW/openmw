#!/bin/sh -e

git clone https://github.com/recastnavigation/recastnavigation.git
cd recastnavigation
mkdir build
cd build
cmake \
    -DCMAKE_BUILD_TYPE="${CONFIGURATION}" \
    -DRECASTNAVIGATION_DEMO=OFF \
    -DRECASTNAVIGATION_TESTS=OFF \
    -DRECASTNAVIGATION_EXAMPLES=OFF \
    -DRECASTNAVIGATION_STATIC=ON \
    -DCMAKE_INSTALL_PREFIX=. \
    -G "${GENERATOR}" \
    ..
cmake --build . --config "${CONFIGURATION}"
cmake --build . --target install --config "${CONFIGURATION}"
