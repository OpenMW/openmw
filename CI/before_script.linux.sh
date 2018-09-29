#!/bin/sh -e

free -m

env GENERATOR='Unix Makefiles' CONFIGURATION=Release CI/build_googletest.sh
GOOGLETEST_DIR="$(pwd)/googletest/build"

mkdir build
cd build
export CODE_COVERAGE=1
if [ "${CC}" = "clang" ]; then export CODE_COVERAGE=0; fi
${ANALYZE}cmake \
    -DBUILD_WITH_CODE_COVERAGE=${CODE_COVERAGE} \
    -DBUILD_UNITTESTS=1 \
    -DBUILD_DOCS=ON \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBINDIR=/usr/games \
    -DCMAKE_BUILD_TYPE="None" \
    -DUSE_SYSTEM_TINYXML=TRUE \
    -DGTEST_ROOT="${GOOGLETEST_DIR}" \
    -DGMOCK_ROOT="${GOOGLETEST_DIR}" \
    ..
