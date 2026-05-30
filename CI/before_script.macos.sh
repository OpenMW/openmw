#!/bin/bash -e

VERBOSE=""
USE_CCACHE=""
KEEP=""
USE_WERROR=""
DEPENDENCIES_ROOT_PATH="/tmp/openmw-deps"

while getopts VCkEd: ARG
do
    case $ARG in
	V )
	    VERBOSE=true ;;

	C )
	    USE_CCACHE=true ;;

	k )
	    KEEP=true ;;

	E )
	    USE_WERROR=true ;;

        d )
            DEPENDENCIES_ROOT_PATH="$OPTARG"
            ;;

	* )
	    cat <<EOF
Usage: $0 [-VCkETd]
Options:
	-C
		Use ccache.
        -d <dir>
                This folder points to the openmw-deps (e.g. /tmp/openmw-deps).
	-k
		Keep the old build directory, default is to delete it.
	-V
		Run verbosely
	-E
		Use warnings as errors (-Werror)
EOF
	    exit 1
	    ;;
    esac
done

if [[ -z $KEEP ]]; then
    if [[ -n $VERBOSE && -d "build" ]]; then
        echo "Deleting existing build directory"
    fi
    rm -fr build
fi

mkdir -p build
cd build

if [[ "${MACOS_AMD64}" ]]; then
    QT_PATH=$(arch -x86_64 /bin/bash -c "qmake -v | sed -rn -e 's/Using Qt version [.0-9]+ in //p'")
else
    QT_PATH=$(qmake -v | sed -rn -e "s/Using Qt version [.0-9]+ in //p")
fi

if [[ -n $VERBOSE ]]; then
    echo "Using Qt path: ${QT_PATH}"
fi

declare -a CMAKE_CONF_OPTS=(
-D CMAKE_C_COMPILER="clang"
-D CMAKE_CXX_COMPILER="clang++"
-DOPENMW_USE_SYSTEM_YAML_CPP=OFF
)

declare -a BUILD_OPTS=(
-D BUILD_OPENMW=TRUE
-D BUILD_OPENCS=TRUE
-D BUILD_ESMTOOL=TRUE
-D BUILD_BSATOOL=TRUE
-D BUILD_ESSIMPORTER=TRUE
-D BUILD_NIFTEST=TRUE
-D BUILD_NAVMESHTOOL=TRUE
-D BUILD_BULLETOBJECTTOOL=TRUE
-G"Unix Makefiles"
)

if [[ "${MACOS_AMD64}" ]]; then
    VCPKG_TARGET_TRIPLET="x64-osx-dynamic"
    CMAKE_CONF_OPTS+=(
        -D CMAKE_OSX_ARCHITECTURES="x86_64"
        -D CMAKE_OSX_DEPLOYMENT_TARGET="13.7"
    )
else
    VCPKG_TARGET_TRIPLET="arm64-osx-dynamic"
    CMAKE_CONF_OPTS+=(
        -D CMAKE_OSX_DEPLOYMENT_TARGET="14.8"
    )
fi

DEPENDENCIES_INSTALLED_PATH="$DEPENDENCIES_ROOT_PATH/installed/$VCPKG_TARGET_TRIPLET"

CMAKE_CONF_OPTS+=(
    -D CMAKE_PREFIX_PATH="$DEPENDENCIES_INSTALLED_PATH;$QT_PATH"
    -D collada_dom_DIR="$DEPENDENCIES_INSTALLED_PATH/share/collada-dom"
    -DVCPKG_HOST_TRIPLET="$VCPKG_TARGET_TRIPLET"
    -DVCPKG_TARGET_TRIPLET="$VCPKG_TARGET_TRIPLET"
    -DCMAKE_TOOLCHAIN_FILE="$DEPENDENCIES_ROOT_PATH/scripts/buildsystems/vcpkg.cmake"
)

if [[ "${CMAKE_BUILD_TYPE}" ]]; then
    CMAKE_CONF_OPTS+=(
        -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    )
else
    CMAKE_CONF_OPTS+=(
        -D CMAKE_BUILD_TYPE=RelWithDebInfo
    )
fi

if [[ -n $USE_CCACHE ]]; then
    CMAKE_CONF_OPTS+=(
        -D CMAKE_C_COMPILER_LAUNCHER="ccache"
        -D CMAKE_CXX_COMPILER_LAUNCHER="ccache"
    )
fi

if [[ -n $USE_WERROR ]]; then
    CMAKE_CONF_OPTS+=(
        -D OPENMW_CXX_FLAGS="-Werror"
    )
fi

if [[ -n $VERBOSE ]]; then
    echo CMake arguments: \
        "${CMAKE_CONF_OPTS[@]}" \
        "${BUILD_OPTS[@]}" \
        ..
fi

cmake \
    "${CMAKE_CONF_OPTS[@]}" \
    "${BUILD_OPTS[@]}" \
    ..
