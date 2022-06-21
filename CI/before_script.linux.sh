#!/bin/bash

set -xeo pipefail

free -m

# Silence a git warning
git config --global advice.detachedHead false

BUILD_UNITTESTS=OFF
BUILD_BENCHMARKS=OFF

if [[ "${BUILD_TESTS_ONLY}" ]]; then
    export GOOGLETEST_DIR="${PWD}/googletest/build/install"
    env GENERATOR='Unix Makefiles' CONFIGURATION=Release CI/build_googletest.sh
    BUILD_UNITTESTS=ON
    BUILD_BENCHMARKS=ON
fi

# setup our basic cmake build options
declare -a CMAKE_CONF_OPTS=(
    -DCMAKE_C_COMPILER="${CC:-/usr/bin/cc}"
    -DCMAKE_CXX_COMPILER="${CXX:-/usr/bin/c++}"
    -DCMAKE_C_COMPILER_LAUNCHER=ccache
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
    -DCMAKE_INSTALL_PREFIX=install
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
    -DBUILD_SHARED_LIBS=OFF
    -DUSE_SYSTEM_TINYXML=ON
    -DOPENMW_USE_SYSTEM_RECASTNAVIGATION=ON
    -DCMAKE_INSTALL_PREFIX=install
    -DOPENMW_CXX_FLAGS="-Werror -Werror=implicit-fallthrough"  # flags specific to OpenMW project
)

if [[ $CI_OPENMW_USE_STATIC_DEPS ]]; then
    CMAKE_CONF_OPTS+=(
        -DOPENMW_USE_SYSTEM_MYGUI=OFF
        -DOPENMW_USE_SYSTEM_OSG=OFF
        -DOPENMW_USE_SYSTEM_BULLET=OFF
        -DOPENMW_USE_SYSTEM_SQLITE3=OFF
        -DOPENMW_USE_SYSTEM_RECASTNAVIGATION=OFF
    )
fi

if [[ $CI_CLANG_TIDY ]]; then
	CMAKE_CONF_OPTS+=(
	      -DCMAKE_CXX_CLANG_TIDY="clang-tidy;-checks=-*,boost-*,clang-analyzer-*,concurrency-*,performance-*,-header-filter=.*,bugprone-*,misc-definitions-in-headers,misc-misplaced-const,misc-redundant-expression,-bugprone-narrowing-conversions"
	)
fi


if [[ "${CMAKE_BUILD_TYPE}" ]]; then
    CMAKE_CONF_OPTS+=(
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    )
fi

if [[ "${CMAKE_CXX_FLAGS_DEBUG}" ]]; then
    CMAKE_CONF_OPTS+=(
        -DCMAKE_CXX_FLAGS_DEBUG="${CMAKE_CXX_FLAGS_DEBUG}"
    )
fi

if [[ "${CMAKE_EXE_LINKER_FLAGS}" ]]; then
    CMAKE_CONF_OPTS+=(
        -DCMAKE_EXE_LINKER_FLAGS="${CMAKE_EXE_LINKER_FLAGS}"
    )
fi

if [[ "${BUILD_WITH_CODE_COVERAGE}" ]]; then
    CMAKE_CONF_OPTS+=(
        -DBUILD_WITH_CODE_COVERAGE="${BUILD_WITH_CODE_COVERAGE}"
    )
fi

mkdir -p build
cd build

if [[ "${BUILD_TESTS_ONLY}" ]]; then

    # flags specific to our test suite
    CXX_FLAGS="-Wno-error=deprecated-declarations -Wno-error=nonnull -Wno-error=deprecated-copy"
    if [[ "${CXX}" == 'clang++' ]]; then
        CXX_FLAGS="${CXX_FLAGS} -Wno-error=unused-lambda-capture -Wno-error=gnu-zero-variadic-macro-arguments"
    fi
    CMAKE_CONF_OPTS+=(
        -DCMAKE_CXX_FLAGS="${CXX_FLAGS}"
    )

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
        -DBUILD_NAVMESHTOOL=OFF \
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
