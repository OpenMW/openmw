#!/bin/sh -e

if [ ! -d recastnavigation ]; then
    git clone https://github.com/recastnavigation/recastnavigation.git
else
    cd recastnavigation
    git pull
    cd ..
fi

cd recastnavigation
SOURCE_DIR="$(pwd)"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
printf "Configuring... "
cmake \
    -DCMAKE_BUILD_TYPE="${CONFIGURATION}" \
    -DRECASTNAVIGATION_DEMO=OFF \
    -DRECASTNAVIGATION_TESTS=OFF \
    -DRECASTNAVIGATION_EXAMPLES=OFF \
    -DRECASTNAVIGATION_STATIC=ON \
    -DCMAKE_INSTALL_PREFIX=. \
    -G "${GENERATOR}" \
    "${SOURCE_DIR}" \
    >/dev/null
printf "Done. Building... "
cmake --build . --config "${CONFIGURATION}" >/dev/null
printf "Done. Installing... "
cmake --build . --target install --config "${CONFIGURATION}" >/dev/null
printf "Done."
