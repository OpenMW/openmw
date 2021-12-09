#!/bin/bash

set -xeo pipefail

free -m

BUILD_UNITTESTS=OFF
BUILD_BENCHMARKS=OFF

if [[ "${BUILD_TESTS_ONLY}" ]]; then
    export GOOGLETEST_DIR="${PWD}/googletest/build/install"
    env GENERATOR='Unix Makefiles' CONFIGURATION=Release CI/build_googletest.sh
    BUILD_UNITTESTS=ON
    BUILD_BENCHMARKS=ON
fi

CXX_FLAGS='-Werror -Wno-error=deprecated-declarations -Wno-error=nonnull -Wno-error=deprecated-copy'

if [[ "${CXX}" == 'clang++' ]]; then
    CXX_FLAGS="${CXX_FLAGS} -Wno-error=unused-lambda-capture -Wno-error=gnu-zero-variadic-macro-arguments"
fi

declare -a CMAKE_CONF_OPTS=(
    -DCMAKE_C_COMPILER="${CC:-/usr/bin/cc}"
    -DCMAKE_CXX_COMPILER="${CXX:-/usr/bin/c++}"
    -DCMAKE_C_COMPILER_LAUNCHER=ccache
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
    -DCMAKE_INSTALL_PREFIX=install
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
    -DBUILD_SHARED_LIBS=OFF
    -DUSE_SYSTEM_TINYXML=ON
    -DCMAKE_INSTALL_PREFIX=install
    -DCMAKE_C_FLAGS='-Werror'
    -DCMAKE_CXX_FLAGS="${CXX_FLAGS}"
    -DOPENMW_CXX_FLAGS="-Werror=implicit-fallthrough"
)

if [[ $CI_OPENMW_USE_STATIC_DEPS ]]; then
    CMAKE_CONF_OPTS+=(
        -DOPENMW_USE_SYSTEM_MYGUI=OFF
        -DOPENMW_USE_SYSTEM_OSG=OFF
        -DOPENMW_USE_SYSTEM_BULLET=OFF
        -DOPENMW_USE_SYSTEM_SQLITE3=OFF
    )
fi

if [[ $CI_CLANG_TIDY ]]; then
	CMAKE_CONF_OPTS+=(
	      -DCMAKE_CXX_CLANG_TIDY='clang-tidy;-checks=-*,boost-*,clang-analyzer-*,concurrency-*,performance-*,-header-filter=.*,bugprone-*,misc-definitions-in-headers,misc-misplaced-const,misc-redundant-expression'
	)
fi


if [[ "${CMAKE_BUILD_TYPE}" ]]; then
    CMAKE_CONF_OPTS+=(
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    )
fi

mkdir -p build
cd build

if [[ "${BUILD_TESTS_ONLY}" ]]; then
    ${ANALYZE} cmake \
        "${CMAKE_CONF_OPTS[@]}" \
        -DBUILD_OPENMW=OFF \
        -DBUILD_BSATOOL=OFF \
        -DBUILD_ESMTOOL=OFF \
        -DBUILD_LAUNCHER=OFF \
        -DBUILD_MWINIIMPORTER=OFF \
        -DBUILD_ESSIMPORTER=OFF \
        -DBUILD_OPENCS=OFF \
        -DBUILD_WIZARD=OFF \
        -DBUILD_UNITTESTS=${BUILD_UNITTESTS} \
        -DBUILD_BENCHMARKS=${BUILD_BENCHMARKS} \
        -DGTEST_ROOT="${GOOGLETEST_DIR}" \
        -DGMOCK_ROOT="${GOOGLETEST_DIR}" \
        ..
else
    ${ANALYZE} cmake \
        "${CMAKE_CONF_OPTS[@]}" \
        ..
fi
