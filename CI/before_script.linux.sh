#!/bin/bash

set -xeo pipefail

BUILD_DIR_SUFFIX=""
USE_CCACHE=""

while [ $# -gt 0 ]; do
    ARGSTR=$1
    shift

    if [ "${ARGSTR:0:1}" != "-" ]; then
        echo "Unknown argument $ARGSTR"
        echo "Try '$0 -h'"
        exit 1
    fi

    for (( i=1; i<${#ARGSTR}; i++ )); do
        ARG=${ARGSTR:$i:1}
        case $ARG in
            B )
                BUILD_DIR_SUFFIX=$1
                shift ;;
            C )
                USE_CCACHE=true ;;

            h )
                cat <<EOF
Usage: $0 [-Ch]
Options:
    -B <your string here>
        Suffix for build directory
    -C
        Use ccache.
    -h
        Show this message.
EOF
                exit 0
                ;;

            * )
                echo "Unknown argument $ARG."
                echo "Try '$0 -h'"
                exit 1 ;;
        esac
    done
done

# setup our basic cmake build options
declare -a CMAKE_CONF_OPTS=(
    -DCMAKE_C_COMPILER="${CC:-/usr/bin/cc}"
    -DCMAKE_CXX_COMPILER="${CXX:-/usr/bin/c++}"
    -DCMAKE_INSTALL_PREFIX=install
    -DOPENMW_CXX_FLAGS="${OPENMW_CXX_FLAGS}"  # flags specific to OpenMW project
)

if [[ "${CMAKE_EXE_LINKER_FLAGS}" ]]; then
    CMAKE_CONF_OPTS+=(
        -DCMAKE_EXE_LINKER_FLAGS="${CMAKE_EXE_LINKER_FLAGS}"
    )
fi

if [[ $CI_OPENMW_USE_STATIC_DEPS ]]; then
    CMAKE_CONF_OPTS+=(
        -DOPENMW_USE_SYSTEM_MYGUI=OFF
        -DOPENMW_USE_SYSTEM_OSG=OFF
        -DOPENMW_USE_SYSTEM_BULLET=OFF
        -DOPENMW_USE_SYSTEM_SQLITE3=OFF
        -DOPENMW_USE_SYSTEM_RECASTNAVIGATION=OFF
    )
fi

if [[ $OPENMW_LOCAL_INSTALL ]]; then
    CMAKE_CONF_OPTS+=(
        -DOPENMW_LOCAL_INSTALL=ON
    )
fi

if [[ $CI_OPENMW_USE_VCPKG ]]; then
    . ./CI/linux/deps_versions.sh

    DEPENDENCIES_ROOT_PATH="deps/${VCPKG_FILE}-${VCPKG_TAG}"
    VCPKG_TARGET_TRIPLET="x64-linux-dynamic"
    QT_PATH="deps/Qt/$QT_VER/gcc_64"

    DEPENDENCIES_INSTALLED_PATH="$DEPENDENCIES_ROOT_PATH/installed/$VCPKG_TARGET_TRIPLET"

    CMAKE_CONF_OPTS+=(
        -D CMAKE_PREFIX_PATH="$DEPENDENCIES_INSTALLED_PATH;$QT_PATH"
        -D collada_dom_DIR="$DEPENDENCIES_INSTALLED_PATH/share/collada-dom"
        -DOPENMW_USE_SYSTEM_RECASTNAVIGATION=OFF
        -DOPENMW_USE_SYSTEM_SQLITE3=OFF
        -DOPENMW_USE_SYSTEM_YAML_CPP=OFF
        -DDEPLOY_QT_TRANSLATIONS=ON
        -DOPENMW_DEPLOY_RUNTIME_DEPENDENCIES=ON
        -DVCPKG_HOST_TRIPLET="$VCPKG_TARGET_TRIPLET"
        -DVCPKG_TARGET_TRIPLET="$VCPKG_TARGET_TRIPLET"
        -DCMAKE_TOOLCHAIN_FILE="$DEPENDENCIES_ROOT_PATH/scripts/buildsystems/vcpkg.cmake"
    )
else
    CMAKE_CONF_OPTS+=(
        -DBUILD_SHARED_LIBS="${BUILD_SHARED_LIBS:-OFF}"
        -DUSE_SYSTEM_TINYXML=ON
        -DOPENMW_USE_SYSTEM_RECASTNAVIGATION=ON
    )
fi

if [[ -n $USE_CCACHE ]]; then
    CMAKE_CONF_OPTS+=(
        -D CMAKE_C_COMPILER_LAUNCHER="ccache"
        -D CMAKE_CXX_COMPILER_LAUNCHER="ccache"
    )
fi

if [[ $CI_CLANG_TIDY ]]; then
    CMAKE_CONF_OPTS+=(
        -DCMAKE_CXX_CLANG_TIDY=${CMAKE_CXX_CLANG_TIDY}
        -DBUILD_COMPONENTS_TESTS=ON
        -DBUILD_OPENMW_TESTS=ON
        -DBUILD_OPENCS_TESTS=ON
        -DBUILD_BENCHMARKS=ON
    )
fi

if [[ "${CMAKE_BUILD_TYPE}" ]]; then
    CMAKE_CONF_OPTS+=(
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    )
else
    CMAKE_CONF_OPTS+=(
        -DCMAKE_BUILD_TYPE=RelWithDebInfo
    )
fi

if [[ "${CMAKE_CXX_FLAGS_DEBUG}" ]]; then
    CMAKE_CONF_OPTS+=(
        -DCMAKE_CXX_FLAGS_DEBUG="${CMAKE_CXX_FLAGS_DEBUG}"
    )
fi

if [[ "${BUILD_WITH_CODE_COVERAGE}" ]]; then
    CMAKE_CONF_OPTS+=(
        -DBUILD_WITH_CODE_COVERAGE="${BUILD_WITH_CODE_COVERAGE}"
    )
fi

if [[ "${CUSTOM_CPACK_LABEL}" ]]; then
    CMAKE_CONF_OPTS+=(
        -DCUSTOM_CPACK_LABEL="${CUSTOM_CPACK_LABEL}"
    )
fi

mkdir -p "build${BUILD_DIR_SUFFIX}"
cd "build${BUILD_DIR_SUFFIX}"

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
        -DBUILD_BULLETOBJECTTOOL=OFF \
        -DBUILD_NIFTEST=OFF \
        -DBUILD_COMPONENTS_TESTS=ON \
        -DBUILD_OPENMW_TESTS=ON \
        -DBUILD_OPENCS_TESTS=ON \
        -DBUILD_BENCHMARKS=ON \
        ..
elif [[ "${BUILD_OPENMW_ONLY}" ]]; then
    ${ANALYZE} cmake \
        "${CMAKE_CONF_OPTS[@]}" \
        -DBUILD_OPENMW=ON \
        -DBUILD_BSATOOL=OFF \
        -DBUILD_ESMTOOL=OFF \
        -DBUILD_LAUNCHER=OFF \
        -DBUILD_MWINIIMPORTER=OFF \
        -DBUILD_ESSIMPORTER=OFF \
        -DBUILD_OPENCS=OFF \
        -DBUILD_WIZARD=OFF \
        -DBUILD_NAVMESHTOOL=OFF \
        -DBUILD_BULLETOBJECTTOOL=OFF \
        -DBUILD_NIFTEST=OFF \
        ..
else
    ${ANALYZE} cmake \
        "${CMAKE_CONF_OPTS[@]}" \
        ..
fi
