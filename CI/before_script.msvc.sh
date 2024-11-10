#!/bin/bash
# set -x  # turn-on for debugging

function wrappedExit {
	if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
		exit $1
	else
		return $1
	fi
}

MISSINGTOOLS=0

command -v 7z >/dev/null 2>&1 || { echo "Error: 7z (7zip) is not on the path."; MISSINGTOOLS=1; }
command -v cmake >/dev/null 2>&1 || { echo "Error: cmake (CMake) is not on the path."; MISSINGTOOLS=1; }

if [ $MISSINGTOOLS -ne 0 ]; then
	wrappedExit 1
fi

WORKINGDIR="$(pwd)"
case "$WORKINGDIR" in
	*[[:space:]]*)
		echo "Error: Working directory contains spaces."
		wrappedExit 1
		;;
esac

set -euo pipefail

function windowsPathAsUnix {
	if command -v cygpath >/dev/null 2>&1; then
		cygpath -u $1
	else
		echo "$1" | sed "s,\\\\,/,g" | sed "s,\(.\):,/\\1,"
	fi
}

function unixPathAsWindows {
	if command -v cygpath >/dev/null 2>&1; then
		cygpath -w $1
	else
		echo "$1" | sed "s,^/\([^/]\)/,\\1:/," | sed "s,/,\\\\,g"
	fi
}

CI=${CI:-}
STEP=${STEP:-}

VERBOSE=""
STRIP=""
SKIP_DOWNLOAD=""
SKIP_EXTRACT=""
USE_CCACHE=""
KEEP=""
UNITY_BUILD=""
VS_VERSION=""
NMAKE=""
NINJA=""
PDBS=""
PLATFORM=""
CONFIGURATIONS=()
TEST_FRAMEWORK=""
INSTALL_PREFIX="."
BUILD_BENCHMARKS=""
USE_WERROR=""
USE_CLANG_TIDY=""

ACTIVATE_MSVC=""
SINGLE_CONFIG=""

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

			d )
				SKIP_DOWNLOAD=true ;;

			e )
				SKIP_EXTRACT=true ;;

			C )
				USE_CCACHE=true ;;

			k )
				KEEP=true ;;

			u )
				UNITY_BUILD=true ;;

			v )
				VS_VERSION=$1
				shift ;;

			n )
				NMAKE=true ;;

			N )
				NINJA=true ;;

			p )
				PLATFORM=$1
				shift ;;

			P )
				PDBS=true ;;

			c )
				CONFIGURATIONS+=( $1 )
				shift ;;

			t )
				TEST_FRAMEWORK=true ;;

			i )
				INSTALL_PREFIX=$(echo "$1" | sed 's;\\;/;g' | sed -E 's;/+;/;g')
				shift ;;

			b )
				BUILD_BENCHMARKS=true ;;

			E )
				USE_WERROR=true ;;

			T )
				USE_CLANG_TIDY=true ;;

			h )
				cat <<EOF
Usage: $0 [-cdehkpuvVi]
Options:
	-c <Release/Debug/RelWithDebInfo>
		Set the configuration, can also be set with environment variable CONFIGURATION.
		For mutli-config generators, this is ignored, and all configurations are set up.
		For single-config generators, several configurations can be set up at once by specifying -c multiple times.
	-C
		Use ccache.
	-d
		Skip checking the downloads.
	-e
		Skip extracting dependencies.
	-h
		Show this message.
	-k
		Keep the old build directory, default is to delete it.
	-p <Win32/Win64>
		Set the build platform, can also be set with environment variable PLATFORM.
	-t
		Build unit tests / Google test
	-u
		Configure for unity builds.
	-v <2019/2022>
		Choose the Visual Studio version to use.
	-n
		Produce NMake makefiles instead of a Visual Studio solution. Cannot be used with -N.
	-N
		Produce Ninja (multi-config if CMake is new enough to support it) files instead of a Visual Studio solution. Cannot be used with -n..
	-P
		Download debug symbols where available
	-V
		Run verbosely
	-i
		CMake install prefix
	-b
		Build benchmarks
	-M
		Use a multiview build of OSG
	-E
		Use warnings as errors (/WX)
	-T
		Run clang-tidy
EOF
				wrappedExit 0
				;;

			* )
				echo "Unknown argument $ARG."
				echo "Try '$0 -h'"
				wrappedExit 1 ;;
		esac
	done
done

if [ -n "$NMAKE" ] || [ -n "$NINJA" ]; then
	if [ -n "$NMAKE" ] && [ -n "$NINJA" ]; then
		echo "Cannot run in NMake and Ninja mode at the same time."
		wrappedExit 1
	fi
	ACTIVATE_MSVC=true
fi

if [ -z $VERBOSE ]; then
	STRIP="> /dev/null 2>&1"
fi

DIR=$(windowsPathAsUnix "${BASH_SOURCE[0]}")
cd $(dirname "$DIR")/..

run_cmd() {
	CMD="$1"
	shift

	if [ -z $VERBOSE ]; then
		RET=0
		eval $CMD $@ > output.log 2>&1 || RET=$?

		if [ $RET -ne 0 ]; then
			echo "Command $CMD failed, output can be found in $(real_pwd)/output.log"
		else
			rm output.log
		fi

		return $RET
	else
		RET=0
		eval $CMD $@ || RET=$?
		return $RET
	fi
}

download() {
	if [ $# -lt 3 ]; then
		echo "Invalid parameters to download."
		return 1
	fi

	NAME=$1
	shift

	echo "$NAME..."

	while [ $# -gt 1 ]; do
		URL=$1
		FILE=$2
		shift
		shift

		if ! [ -f $FILE ]; then
			printf "  Downloading $FILE... "

			if [ -z $VERBOSE ]; then
				RET=0
				curl --silent --fail --retry 10 -Ly 5 -o $FILE $URL || RET=$?
			else
				RET=0
				curl --fail --retry 10 -Ly 5 -o $FILE $URL || RET=$?
			fi

			if [ $RET -ne 0 ]; then
				echo "Failed!"
				wrappedExit $RET
			else
				echo "Done."
			fi
		else
			echo "  $FILE exists, skipping."
		fi
	done

	if [ $# -ne 0 ]; then
		echo "Missing parameter."
	fi
}

MANIFEST_FILE=""
download_from_manifest() {
	if [ $# -ne 1 ]; then
		echo "Invalid parameters to download_from_manifest."
		return 1
	fi
	{ read -r URL && read -r HASH FILE; } < $1
	if [ -z $SKIP_DOWNLOAD ]; then
		download "${FILE:?}" "${URL:?}" "${FILE:?}"
	fi
	echo "${HASH:?}  ${FILE:?}" | shasum -a 512 --check
	MANIFEST_FILE="${FILE:?}"
}

real_pwd() {
	if type cygpath >/dev/null 2>&1; then
		cygpath -am "$PWD"
	else
		pwd # not git bash, Cygwin or the like
	fi
}

CMAKE_OPTS=""
add_cmake_opts() {
	CMAKE_OPTS="$CMAKE_OPTS $@"
}

declare -A RUNTIME_DLLS
RUNTIME_DLLS["Release"]=""
RUNTIME_DLLS["Debug"]=""
RUNTIME_DLLS["RelWithDebInfo"]=""
add_runtime_dlls() {
	local CONFIG=$1
	shift
	RUNTIME_DLLS[$CONFIG]="${RUNTIME_DLLS[$CONFIG]} $@"
}

declare -A OSG_PLUGINS
OSG_PLUGINS["Release"]=""
OSG_PLUGINS["Debug"]=""
OSG_PLUGINS["RelWithDebInfo"]=""
add_osg_dlls() {
	local CONFIG=$1
	shift
	OSG_PLUGINS[$CONFIG]="${OSG_PLUGINS[$CONFIG]} $@"
}

declare -A QT_PLATFORMS
QT_PLATFORMS["Release"]=""
QT_PLATFORMS["Debug"]=""
QT_PLATFORMS["RelWithDebInfo"]=""
add_qt_platform_dlls() {
	local CONFIG=$1
	shift
	QT_PLATFORMS[$CONFIG]="${QT_PLATFORMS[$CONFIG]} $@"
}

declare -A QT_STYLES
QT_STYLES["Release"]=""
QT_STYLES["Debug"]=""
QT_STYLES["RelWithDebInfo"]=""
add_qt_style_dlls() {
	local CONFIG=$1
	shift
	QT_STYLES[$CONFIG]="${QT_STYLES[$CONFIG]} $@"
}

declare -A QT_IMAGEFORMATS
QT_IMAGEFORMATS["Release"]=""
QT_IMAGEFORMATS["Debug"]=""
QT_IMAGEFORMATS["RelWithDebInfo"]=""
add_qt_image_dlls() {
	local CONFIG=$1
	shift
	QT_IMAGEFORMATS[$CONFIG]="${QT_IMAGEFORMATS[$CONFIG]} $@"
}

declare -A QT_ICONENGINES
QT_ICONENGINES["Release"]=""
QT_ICONENGINES["Debug"]=""
QT_ICONENGINES["RelWithDebInfo"]=""
add_qt_icon_dlls() {
	local CONFIG=$1
	shift
	QT_ICONENGINES[$CONFIG]="${QT_ICONENGINES[$CONFIG]} $@"
}

if [ -z $PLATFORM ]; then
	PLATFORM="$(uname -m)"
fi

if [ -z $VS_VERSION ]; then
	VS_VERSION="2019"
fi

case $VS_VERSION in
	17|17.0|2022 )
		GENERATOR="Visual Studio 17 2022"
		MSVC_TOOLSET="vc143"
		MSVC_REAL_VER="17"
		MSVC_DISPLAY_YEAR="2022"

		QT_MSVC_YEAR="2019"
		;;

	16|16.0|2019 )
		GENERATOR="Visual Studio 16 2019"
		MSVC_TOOLSET="vc142"
		MSVC_REAL_VER="16"
		MSVC_DISPLAY_YEAR="2019"

		QT_MSVC_YEAR="2019"
		;;

	15|15.0|2017 )
		echo "Visual Studio 2017 is no longer supported"
		wrappedExit 1
		;;

	14|14.0|2015 )
		echo "Visual Studio 2015 is no longer supported"
		wrappedExit 1
		;;

	12|12.0|2013 )
		echo "Visual Studio 2013 is no longer supported"
		wrappedExit 1
		;;
esac

case $PLATFORM in
	x64|x86_64|x86-64|win64|Win64 )
		ARCHNAME="x86-64"
		ARCHSUFFIX="64"
		BITS="64"
		;;

	x32|x86|i686|i386|win32|Win32 )
		ARCHNAME="x86"
		ARCHSUFFIX="86"
		BITS="32"
		;;

	* )
		echo "Unknown platform $PLATFORM."
		wrappedExit 1
		;;
esac

if [ -n "$NMAKE" ]; then
	GENERATOR="NMake Makefiles"
	SINGLE_CONFIG=true
fi

if [ -n "$NINJA" ]; then
	GENERATOR="Ninja Multi-Config"
	if ! cmake -E capabilities | grep -F "$GENERATOR" > /dev/null; then
		SINGLE_CONFIG=true
		GENERATOR="Ninja"
	fi
fi

if [ -n "$SINGLE_CONFIG" ]; then
	if [ ${#CONFIGURATIONS[@]} -eq 0 ]; then
		if [ -n "${CONFIGURATION:-}" ]; then
			CONFIGURATIONS=("$CONFIGURATION")
		else
			CONFIGURATIONS=("Debug")
		fi
	elif [ ${#CONFIGURATIONS[@]} -ne 1 ]; then
		# It's simplest just to recursively call the script a few times.
		RECURSIVE_OPTIONS=()
		if [ -n "$VERBOSE" ]; then
			RECURSIVE_OPTIONS+=("-V")
		fi
		if [ -n "$SKIP_DOWNLOAD" ]; then
			RECURSIVE_OPTIONS+=("-d")
		fi
		if [ -n "$SKIP_EXTRACT" ]; then
			RECURSIVE_OPTIONS+=("-e")
		fi
		if [ -n "$KEEP" ]; then
			RECURSIVE_OPTIONS+=("-k")
		fi
		if [ -n "$UNITY_BUILD" ]; then
			RECURSIVE_OPTIONS+=("-u")
		fi
		if [ -n "$NMAKE" ]; then
			RECURSIVE_OPTIONS+=("-n")
		fi
		if [ -n "$NINJA" ]; then
			RECURSIVE_OPTIONS+=("-N")
		fi
		if [ -n "$PDBS" ]; then
			RECURSIVE_OPTIONS+=("-P")
		fi
		if [ -n "$TEST_FRAMEWORK" ]; then
			RECURSIVE_OPTIONS+=("-t")
		fi
		RECURSIVE_OPTIONS+=("-v $VS_VERSION")
		RECURSIVE_OPTIONS+=("-p $PLATFORM")
		RECURSIVE_OPTIONS+=("-i '$INSTALL_PREFIX'")

		for config in ${CONFIGURATIONS[@]}; do
			$0 ${RECURSIVE_OPTIONS[@]} -c $config
		done

		wrappedExit 1
	fi
else
	if [ ${#CONFIGURATIONS[@]} -ne 0 ]; then
		echo "Ignoring configurations argument - generator is multi-config"
	fi
	CONFIGURATIONS=("Release" "Debug" "RelWithDebInfo")
fi

for i in ${!CONFIGURATIONS[@]}; do
	case ${CONFIGURATIONS[$i]} in
		debug|Debug|DEBUG )
			CONFIGURATIONS[$i]=Debug
			;;

		release|Release|RELEASE )
			CONFIGURATIONS[$i]=Release
			;;

		relwithdebinfo|RelWithDebInfo|RELWITHDEBINFO )
			CONFIGURATIONS[$i]=RelWithDebInfo
			;;
	esac
done

if [ -z "$NMAKE" ] && [ -z "$NINJA" ]; then
	if [ $BITS -eq 64 ]; then
		add_cmake_opts "-G\"$GENERATOR\" -A x64"
	else
		add_cmake_opts "-G\"$GENERATOR\" -A Win32"
	fi
else
	add_cmake_opts "-G\"$GENERATOR\""
fi

if [ -n "$SINGLE_CONFIG" ]; then
	add_cmake_opts "-DCMAKE_BUILD_TYPE=${CONFIGURATIONS[0]}"
fi

if [[ -n "$UNITY_BUILD" ]]; then
	add_cmake_opts "-DOPENMW_UNITY_BUILD=True"
fi

if [ -n "$USE_CCACHE" ]; then
	if [ -n "$NMAKE" ] || [ -n "$NINJA" ]; then
		add_cmake_opts "-DCMAKE_C_COMPILER_LAUNCHER=ccache  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DPRECOMPILE_HEADERS_WITH_MSVC=OFF"
	else
		echo "Ignoring -C (CCache) as it is incompatible with Visual Studio CMake generators"
	fi
fi

# turn on LTO by default
add_cmake_opts "-DOPENMW_LTO_BUILD=True"

if [[ -n "$USE_WERROR" ]]; then
  add_cmake_opts "-DOPENMW_MSVC_WERROR=ON"
fi

if [[ -n "$USE_CLANG_TIDY" ]]; then
  add_cmake_opts "-DCMAKE_CXX_CLANG_TIDY=\"clang-tidy --warnings-as-errors=*\""
fi

QT_VER='6.6.3'
AQT_VERSION='v3.1.15'

VCPKG_TAG="2024-11-10"
VCPKG_PATH="vcpkg-x64-${VS_VERSION:?}-${VCPKG_TAG:?}"
VCPKG_PDB_PATH="vcpkg-x64-${VS_VERSION:?}-pdb-${VCPKG_TAG:?}"
VCPKG_MANIFEST="${VCPKG_PATH:?}.txt"
VCPKG_PDB_MANIFEST="${VCPKG_PDB_PATH:?}.txt"

echo
echo "==================================="
echo "Starting prebuild on MSVC${MSVC_DISPLAY_YEAR} WIN${BITS}"
echo "==================================="
echo

mkdir -p deps
cd deps

DEPS="$(pwd)"

if [ -z $SKIP_DOWNLOAD ]; then
	echo "Downloading dependency packages."
	echo

	DEPS_BASE_URL="https://gitlab.com/OpenMW/openmw-deps/-/raw/main/windows"

	download "${VCPKG_MANIFEST:?}" \
		"${DEPS_BASE_URL}/${VCPKG_MANIFEST:?}" \
		"${VCPKG_MANIFEST:?}"

	if [ -n "${VCPKG_PDB_MANIFEST:?}" ]; then
		download "${VCPKG_PDB_PATH:?}" \
			"${DEPS_BASE_URL}/${VCPKG_PDB_MANIFEST:?}" \
			"${VCPKG_PDB_MANIFEST:?}"
	fi
fi

cd .. #/..

# Set up dependencies
BUILD_DIR="MSVC${MSVC_DISPLAY_YEAR}_${BITS}"

if [ -n "$NMAKE" ]; then
	BUILD_DIR="${BUILD_DIR}_NMake"
elif [ -n "$NINJA" ]; then
	BUILD_DIR="${BUILD_DIR}_Ninja"
fi

if [ -n "$SINGLE_CONFIG" ]; then
	BUILD_DIR="${BUILD_DIR}_${CONFIGURATIONS[0]}"
fi

if [ -z $KEEP ]; then
	echo
	echo "(Re)Creating build directory."

	rm -rf "$BUILD_DIR"
fi

mkdir -p "${BUILD_DIR}/deps"
cd "${BUILD_DIR}/deps"

DEPS_INSTALL="$(pwd)"
cd $DEPS

echo
echo "Extracting dependencies, this might take a while..."
echo "---------------------------------------------------"
echo

cd $DEPS
echo
printf "vcpkg packages ${VCPKG_TAG:?}... "
{
	if [[ -d "${VCPKG_PATH:?}" ]]; then
		printf "Exists. "
	else
		download_from_manifest "${VCPKG_MANIFEST:?}"
		eval 7z x -y -o"${VCPKG_PATH:?}" "${MANIFEST_FILE:?}" ${STRIP}
	fi
	if [ -n "${PDBS}" ]; then
		if [[ -d "${VCPKG_PDB_PATH:?}" ]]; then
			printf "PDB exists. "
		else
			download_from_manifest "${VCPKG_PDB_MANIFEST:?}"
			eval 7z x -y -o"${VCPKG_PDB_PATH:?}" "${MANIFEST_FILE:?}" ${STRIP}
		fi
	fi

	add_cmake_opts -DCMAKE_TOOLCHAIN_FILE="$(real_pwd)/${VCPKG_PATH:?}/scripts/buildsystems/vcpkg.cmake"
	add_cmake_opts -DLuaJit_INCLUDE_DIR="$(real_pwd)/${VCPKG_PATH:?}/installed/x64-windows/include/luajit"
	add_cmake_opts -DLuaJit_LIBRARY="$(real_pwd)/${VCPKG_PATH:?}/installed/x64-windows/lib/lua51.lib"

	for CONFIGURATION in ${CONFIGURATIONS[@]}; do
		if [[ ${CONFIGURATION:?} == "Debug" ]]; then
			VCPKG_DLL_BIN="$(pwd)/${VCPKG_PATH:?}/installed/x64-windows/debug/bin"

			add_runtime_dlls ${CONFIGURATION:?} "${VCPKG_DLL_BIN:?}/Debug/MyGUIEngine_d.dll"
		else
			VCPKG_DLL_BIN="$(pwd)/${VCPKG_PATH:?}/installed/x64-windows/bin"

			add_runtime_dlls ${CONFIGURATION:?} "${VCPKG_DLL_BIN:?}/Release/MyGUIEngine.dll"
		fi

		add_osg_dlls ${CONFIGURATION:?} "${VCPKG_DLL_BIN:?}/osgPlugins-3.6.5/*.dll"
		add_runtime_dlls ${CONFIGURATION:?} "${VCPKG_DLL_BIN:?}/*.dll"
	done

	echo Done.
}

cd $DEPS
echo
printf "Qt ${QT_VER}... "
{
	if [ $BITS -eq 64 ]; then
		SUFFIX="_64"
	else
		SUFFIX=""
	fi

	cd $DEPS_INSTALL


	QT_SDK="$(real_pwd)/Qt/${QT_VER}/msvc${QT_MSVC_YEAR}${SUFFIX}"

	if [ -d "Qt/${QT_VER}" ]; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		pushd "$DEPS" > /dev/null
		if ! [ -f "aqt_x64-${AQT_VERSION}.exe" ]; then
			download "aqt ${AQT_VERSION}"\
				"https://github.com/miurahr/aqtinstall/releases/download/${AQT_VERSION}/aqt_x64.exe" \
				"aqt_x64-${AQT_VERSION}.exe"
		fi
		popd > /dev/null

		rm -rf Qt

		mkdir Qt
		cd Qt

		run_cmd "${DEPS}/aqt_x64-${AQT_VERSION}.exe" install-qt windows desktop ${QT_VER} "win${BITS}_msvc${QT_MSVC_YEAR}${SUFFIX}"

		printf "  Cleaning up extraneous data... "
		rm -rf Qt/{aqtinstall.log,Tools}

		echo Done.
	fi

	QT_MAJOR_VER=$(echo "${QT_VER}" | awk -F '[.]' '{printf "%d", $1}')
	QT_MINOR_VER=$(echo "${QT_VER}" | awk -F '[.]' '{printf "%d", $2}')

	cd $QT_SDK
	for CONFIGURATION in ${CONFIGURATIONS[@]}; do
		if [ $CONFIGURATION == "Debug" ]; then
			DLLSUFFIX="d"
		else
			DLLSUFFIX=""
		fi

		if [ "${QT_MAJOR_VER}" -eq 6 ]; then
			add_runtime_dlls $CONFIGURATION "$(pwd)/bin/Qt${QT_MAJOR_VER}"{Core,Gui,Network,OpenGL,OpenGLWidgets,Widgets,Svg}${DLLSUFFIX}.dll

			# Since Qt 6.7.0 plugin is called "qmodernwindowsstyle"
			if [ "${QT_MINOR_VER}" -ge 7 ]; then
				add_qt_style_dlls $CONFIGURATION "$(pwd)/plugins/styles/qmodernwindowsstyle${DLLSUFFIX}.dll"
			else
				add_qt_style_dlls $CONFIGURATION "$(pwd)/plugins/styles/qwindowsvistastyle${DLLSUFFIX}.dll"
			fi
		else
			add_runtime_dlls $CONFIGURATION "$(pwd)/bin/Qt${QT_MAJOR_VER}"{Core,Gui,Network,OpenGL,Widgets,Svg}${DLLSUFFIX}.dll
			add_qt_style_dlls $CONFIGURATION "$(pwd)/plugins/styles/qwindowsvistastyle${DLLSUFFIX}.dll"
		fi

		add_qt_platform_dlls $CONFIGURATION "$(pwd)/plugins/platforms/qwindows${DLLSUFFIX}.dll"
		add_qt_image_dlls $CONFIGURATION "$(pwd)/plugins/imageformats/qsvg${DLLSUFFIX}.dll"
		add_qt_icon_dlls $CONFIGURATION "$(pwd)/plugins/iconengines/qsvgicon${DLLSUFFIX}.dll"
	done
	echo Done.
}

add_cmake_opts -DCMAKE_PREFIX_PATH="\"${QT_SDK}\""

echo
cd $DEPS_INSTALL/..
echo
echo "Setting up OpenMW build..."
add_cmake_opts -DOPENMW_MP_BUILD=on
add_cmake_opts -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"
add_cmake_opts -DOPENMW_USE_SYSTEM_SQLITE3=OFF
add_cmake_opts -DOPENMW_USE_SYSTEM_YAML_CPP=OFF
if [ ! -z $CI ]; then
	case $STEP in
		components )
			echo "  Building subproject: Components."
			add_cmake_opts -DBUILD_ESSIMPORTER=no \
				-DBUILD_LAUNCHER=no \
				-DBUILD_MWINIIMPORTER=no \
				-DBUILD_OPENCS=no \
				-DBUILD_OPENMW=no \
				-DBUILD_WIZARD=no
			;;
		openmw )
			echo "  Building subproject: OpenMW."
			add_cmake_opts -DBUILD_ESSIMPORTER=no \
				-DBUILD_LAUNCHER=no \
				-DBUILD_MWINIIMPORTER=no \
				-DBUILD_OPENCS=no \
				-DBUILD_WIZARD=no
			;;
		opencs )
			echo "  Building subproject: OpenCS."
			add_cmake_opts -DBUILD_ESSIMPORTER=no \
				-DBUILD_LAUNCHER=no \
				-DBUILD_MWINIIMPORTER=no \
				-DBUILD_OPENMW=no \
				-DBUILD_WIZARD=no
			;;
		misc )
			echo "  Building subprojects: Misc."
			add_cmake_opts -DBUILD_OPENCS=no \
				-DBUILD_OPENMW=no
			;;
	esac
fi
# NOTE: Disable this when/if we want to run test cases
#if [ -z $CI ]; then
	for CONFIGURATION in ${CONFIGURATIONS[@]}; do
		echo "- Copying Runtime DLLs for $CONFIGURATION..."
		DLL_PREFIX=""
		if [ -z $SINGLE_CONFIG ]; then
			mkdir -p $CONFIGURATION
			DLL_PREFIX="$CONFIGURATION/"
		fi
		for DLL in ${RUNTIME_DLLS[$CONFIGURATION]}; do
			TARGET="$(basename "$DLL")"
			if [[ "$DLL" == *":"* ]]; then
				originalIFS="$IFS"
				IFS=':'; SPLIT=( ${DLL} ); IFS=$originalIFS
				DLL=${SPLIT[0]}
				TARGET=${SPLIT[1]}
			fi
			echo "    ${TARGET}."
			cp "$DLL" "${DLL_PREFIX}$TARGET"
		done
		echo
		echo "- OSG Plugin DLLs..."
		mkdir -p ${DLL_PREFIX}osgPlugins-3.6.5
		for DLL in ${OSG_PLUGINS[$CONFIGURATION]}; do
			echo "    $(basename $DLL)."
			cp "$DLL" ${DLL_PREFIX}osgPlugins-3.6.5
		done
		echo
		echo "- Qt Platform DLLs..."
		mkdir -p ${DLL_PREFIX}platforms
		for DLL in ${QT_PLATFORMS[$CONFIGURATION]}; do
			echo "    $(basename $DLL)"
			cp "$DLL" "${DLL_PREFIX}platforms"
		done
		echo
		echo "- Qt Style DLLs..."
		mkdir -p ${DLL_PREFIX}styles
		for DLL in ${QT_STYLES[$CONFIGURATION]}; do
			echo "    $(basename $DLL)"
			cp "$DLL" "${DLL_PREFIX}styles"
		done
		echo
		echo "- Qt Image Format DLLs..."
		mkdir -p ${DLL_PREFIX}imageformats
		for DLL in ${QT_IMAGEFORMATS[$CONFIGURATION]}; do
			echo "    $(basename $DLL)"
			cp "$DLL" "${DLL_PREFIX}imageformats"
		done
		echo
		echo "- Qt Icon Engine DLLs..."
		mkdir -p ${DLL_PREFIX}iconengines
		for DLL in ${QT_ICONENGINES[$CONFIGURATION]}; do
			echo "    $(basename $DLL)"
			cp "$DLL" "${DLL_PREFIX}iconengines"
		done
		echo
	done
#fi

if [ "${BUILD_BENCHMARKS}" ]; then
	add_cmake_opts -DBUILD_BENCHMARKS=ON
fi

if [ -n "${TEST_FRAMEWORK}" ]; then
	add_cmake_opts -DBUILD_COMPONENTS_TESTS=ON
	add_cmake_opts -DBUILD_OPENCS_TESTS=ON
	add_cmake_opts -DBUILD_OPENMW_TESTS=ON
fi

if [ -n "$ACTIVATE_MSVC" ]; then
	echo -n "- Activating MSVC in the current shell... "
	command -v vswhere >/dev/null 2>&1 || { echo "Error: vswhere is not on the path."; wrappedExit 1; }

	# There are so many arguments now that I'm going to document them:
	# * products: allow Visual Studio or standalone build tools
	# * version: obvious. Awk helps make a version range by adding one.
	# * property installationPath: only give the installation path.
	# * latest: return only one result if several candidates exist. Prefer the last installed/updated
	# * requires: make sure it's got the MSVC compiler instead of, for example, just the .NET compiler. The .x86.x64 suffix means it's for either, not that it's the x64 on x86 cross compiler as you always get both
	MSVC_INSTALLATION_PATH=$(vswhere -products '*' -version "[$MSVC_REAL_VER,$(awk "BEGIN { print $MSVC_REAL_VER + 1; exit }"))" -property installationPath -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64)
	if [ -z "$MSVC_INSTALLATION_PATH" ]; then
		echo "vswhere was unable to find MSVC $MSVC_DISPLAY_YEAR"
		wrappedExit 1
	fi
	
	echo "@\"${MSVC_INSTALLATION_PATH}\Common7\Tools\VsDevCmd.bat\" -no_logo -arch=$([ $BITS -eq 64 ] && echo "amd64" || echo "x86") -host_arch=$([ $(uname -m) == 'x86_64' ] && echo "amd64" || echo "x86")" > ActivateMSVC.bat
	
	cp "../CI/activate_msvc.sh" .
	sed -i "s/\$MSVC_DISPLAY_YEAR/$MSVC_DISPLAY_YEAR/g" activate_msvc.sh
	source ./activate_msvc.sh
	
	cp "../CI/ActivateMSVC.ps1" .
	sed -i "s/\$MSVC_DISPLAY_YEAR/$MSVC_DISPLAY_YEAR/g" ActivateMSVC.ps1

	echo "done."
	echo
fi

if [ -z $VERBOSE ]; then
	printf -- "- Configuring... "
else
	echo "- cmake .. $CMAKE_OPTS"
fi
RET=0
run_cmd cmake .. $CMAKE_OPTS || RET=$?
if [ -z $VERBOSE ]; then
	if [ $RET -eq 0 ]; then
		echo Done.
	else
		echo Failed.
	fi
fi
if [ $RET -ne 0 ]; then
	wrappedExit $RET
fi

echo "Script completed successfully."
echo "You now have an OpenMW build system at $(unixPathAsWindows "$(pwd)")"

if [ -n "$ACTIVATE_MSVC" ]; then
	echo
	echo "Note: you must manually activate MSVC for the shell in which you want to do the build."
	echo
	echo "Some scripts have been created in the build directory to do so in an existing shell."
	echo "Bash: source activate_msvc.sh"
	echo "CMD: ActivateMSVC.bat"
	echo "PowerShell: ActivateMSVC.ps1"
	echo
	echo "You may find options to launch a Development/Native Tools/Cross Tools shell in your start menu or Visual Studio."
	echo
	if [ $(uname -m) == 'x86_64' ]; then
		if [ $BITS -eq 64 ]; then
			inheritEnvironments=msvc_x64_x64
		else
			inheritEnvironments=msvc_x64
		fi
	else
		if [ $BITS -eq 64 ]; then
			inheritEnvironments=msvc_x86_x64
		else
			inheritEnvironments=msvc_x86
		fi
	fi
	echo "In Visual Studio 15.3 (2017 Update 3) or later, try setting '\"inheritEnvironments\": [ \"$inheritEnvironments\" ]' in CMakeSettings.json to build in the IDE."
fi

wrappedExit $RET
