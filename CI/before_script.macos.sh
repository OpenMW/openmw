#!/bin/bash -e

VERBOSE=""
USE_CCACHE=""
KEEP=""
USE_WERROR=""

while [ $# -gt 0 ]; do
	ARGSTR=$1
	shift

	if [ ${ARGSTR:0:1} != "-" ]; then
		echo "Unknown argument $ARGSTR"
		echo "Try '$0 -h'"
		wrappedExit 1
	fi

	for (( i=1; i<${#ARGSTR}; i++ )); do
		ARG=${ARGSTR:$i:1}
		case $ARG in
			V )
				VERBOSE=true ;;

			C )
				USE_CCACHE=true ;;

			k )
				KEEP=true ;;

			E )
				USE_WERROR=true ;;

			h )
				cat <<EOF
Usage: $0 [-VCkETh]
Options:
	-C
		Use ccache.
	-h
		Show this message.
	-k
		Keep the old build directory, default is to delete it.
	-V
		Run verbosely
	-E
		Use warnings as errors (-Werror)
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

if [[ -z $KEEP ]]; then
    if [[ -n $VERBOSE && -d "build" ]]; then
        echo "Deleting existing build directory"
    fi
    rm -fr build
fi

mkdir -p build
cd build

DEPENDENCIES_ROOT="/tmp/openmw-deps"

if [[ "${MACOS_AMD64}" ]]; then
    QT_PATH=$(arch -x86_64 /bin/bash -c "qmake -v | sed -rn -e 's/Using Qt version [.0-9]+ in //p'")
    ICU_PATH=$(arch -x86_64 /usr/local/bin/brew --prefix icu4c)
    OPENAL_PATH=$(arch -x86_64 /usr/local/bin/brew --prefix openal-soft)
else
    QT_PATH=$(qmake -v | sed -rn -e "s/Using Qt version [.0-9]+ in //p")
    ICU_PATH=$(brew --prefix icu4c)
    OPENAL_PATH=$(brew --prefix openal-soft)
fi

if [[ -n $VERBOSE ]]; then
    echo "Using Qt path: ${QT_PATH}"
    echo "Using ICU path: ${ICU_PATH}"
    echo "Using OpenAL path: ${OPENAL_PATH}"
fi

declare -a CMAKE_CONF_OPTS=(
-D CMAKE_PREFIX_PATH="$DEPENDENCIES_ROOT;$QT_PATH;$OPENAL_PATH"
-D CMAKE_CXX_FLAGS="-stdlib=libc++"
-D CMAKE_C_COMPILER="clang"
-D CMAKE_CXX_COMPILER="clang++"
-D CMAKE_OSX_DEPLOYMENT_TARGET="13.6"
-D OPENMW_USE_SYSTEM_RECASTNAVIGATION=TRUE
-D Boost_INCLUDE_DIR="$DEPENDENCIES_ROOT/include"
-D OSGPlugins_LIB_DIR="$DEPENDENCIES_ROOT/lib/osgPlugins-3.6.5"
-D ICU_ROOT="$ICU_PATH"
-D OPENMW_OSX_DEPLOYMENT=TRUE
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
    CMAKE_CONF_OPTS+=(
        -D CMAKE_OSX_ARCHITECTURES="x86_64"
    )
fi

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
