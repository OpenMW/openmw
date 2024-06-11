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

APPVEYOR=${APPVEYOR:-}
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
GOOGLE_INSTALL_ROOT=""
INSTALL_PREFIX="."
BUILD_BENCHMARKS=""
OSG_MULTIVIEW_BUILD=""
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

			M )
				OSG_MULTIVIEW_BUILD=true ;;

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

if [ -z $APPVEYOR ]; then
	echo "Running prebuild outside of Appveyor."

	DIR=$(windowsPathAsUnix "${BASH_SOURCE[0]}")
	cd $(dirname "$DIR")/..
else
	echo "Running prebuild in Appveyor."

	cd "$APPVEYOR_BUILD_FOLDER"
fi

run_cmd() {
	CMD="$1"
	shift

	if [ -z $VERBOSE ]; then
		RET=0
		eval $CMD $@ > output.log 2>&1 || RET=$?

		if [ $RET -ne 0 ]; then
			if [ -z $APPVEYOR ]; then
				echo "Command $CMD failed, output can be found in $(real_pwd)/output.log"
			else
				echo
				echo "Command $CMD failed;"
				cat output.log
			fi
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
		TOOLSET="vc143"
		MSVC_REAL_VER="17"
		MSVC_VER="14.3"
		MSVC_DISPLAY_YEAR="2022"

		OSG_MSVC_YEAR="2019"
		MYGUI_MSVC_YEAR="2019"
		LUA_MSVC_YEAR="2019"
		QT_MSVC_YEAR="2019"
		BULLET_MSVC_YEAR="2019"

		BOOST_VER="1.80.0"
		BOOST_VER_URL="1_80_0"
		BOOST_VER_SDK="108000"
		;;

	16|16.0|2019 )
		GENERATOR="Visual Studio 16 2019"
		TOOLSET="vc142"
		MSVC_REAL_VER="16"
		MSVC_VER="14.2"
		MSVC_DISPLAY_YEAR="2019"

		OSG_MSVC_YEAR="2019"
		MYGUI_MSVC_YEAR="2019"
		LUA_MSVC_YEAR="2019"
		QT_MSVC_YEAR="2019"
		BULLET_MSVC_YEAR="2019"

		BOOST_VER="1.80.0"
		BOOST_VER_URL="1_80_0"
		BOOST_VER_SDK="108000"
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

BULLET_VER="2.89"
FFMPEG_VER="4.2.2"
ICU_VER="70_1"
LUAJIT_VER="v2.1.0-beta3-452-g7a0cf5fd"
LZ4_VER="1.9.2"
OPENAL_VER="1.23.0"
QT_VER="6.6.2"

OSG_ARCHIVE_NAME="OSGoS 3.6.5"
OSG_ARCHIVE="OSGoS-3.6.5-123-g68c5c573d-msvc${OSG_MSVC_YEAR}-win${BITS}"
OSG_ARCHIVE_REPO_URL="https://gitlab.com/OpenMW/openmw-deps/-/raw/main"
if [[ -n "$OSG_MULTIVIEW_BUILD" ]]; then
	OSG_ARCHIVE_NAME="OSG-3.6-multiview"
	OSG_ARCHIVE="OSG-3.6-multiview-d2ee5aa8-msvc${OSG_MSVC_YEAR}-win${BITS}"
	OSG_ARCHIVE_REPO_URL="https://gitlab.com/madsbuvi/openmw-deps/-/raw/openmw-vr-ovr_multiview"
fi


echo
echo "==================================="
echo "Starting prebuild on MSVC${MSVC_DISPLAY_YEAR} WIN${BITS}"
echo "==================================="
echo

# cd OpenMW/AppVeyor-test
mkdir -p deps
cd deps

DEPS="$(pwd)"

if [ -z $SKIP_DOWNLOAD ]; then
	echo "Downloading dependency packages."
	echo

	# Boost
	if [ -z $APPVEYOR ]; then
		download "Boost ${BOOST_VER}" \
			"https://gitlab.com/OpenMW/openmw-deps/-/raw/main/windows/boost_${BOOST_VER_URL}-msvc-${MSVC_VER}-${BITS}.exe" \
			"boost-${BOOST_VER}-msvc${MSVC_VER}-win${BITS}.exe"
	fi

	# Bullet
	download "Bullet ${BULLET_VER}" \
		"https://gitlab.com/OpenMW/openmw-deps/-/raw/main/windows/Bullet-${BULLET_VER}-msvc${BULLET_MSVC_YEAR}-win${BITS}-double-mt.7z" \
		"Bullet-${BULLET_VER}-msvc${BULLET_MSVC_YEAR}-win${BITS}-double-mt.7z"

	# FFmpeg
	download "FFmpeg ${FFMPEG_VER}" \
	  "https://gitlab.com/OpenMW/openmw-deps/-/raw/main/windows/ffmpeg-${FFMPEG_VER}-win${BITS}.zip" \
		"ffmpeg-${FFMPEG_VER}-win${BITS}.zip" \
		"https://gitlab.com/OpenMW/openmw-deps/-/raw/main/windows/ffmpeg-${FFMPEG_VER}-dev-win${BITS}.zip" \
		"ffmpeg-${FFMPEG_VER}-dev-win${BITS}.zip"

	# MyGUI
	download "MyGUI 3.4.3" \
		"https://gitlab.com/OpenMW/openmw-deps/-/raw/main/windows/MyGUI-3.4.3-msvc${MYGUI_MSVC_YEAR}-win${BITS}.7z" \
		"MyGUI-3.4.3-msvc${MYGUI_MSVC_YEAR}-win${BITS}.7z"

	if [ -n "$PDBS" ]; then
		download "MyGUI symbols" \
			"https://gitlab.com/OpenMW/openmw-deps/-/raw/main/windows/MyGUI-3.4.3-msvc${MYGUI_MSVC_YEAR}-win${BITS}-sym.7z" \
			"MyGUI-3.4.3-msvc${MYGUI_MSVC_YEAR}-win${BITS}-sym.7z"
	fi

	# OpenAL
	download "OpenAL-Soft ${OPENAL_VER}" \
	  "https://gitlab.com/OpenMW/openmw-deps/-/raw/main/windows/OpenAL-Soft-${OPENAL_VER}.zip" \
		"OpenAL-Soft-${OPENAL_VER}.zip"

	# OSGoS
	download "${OSG_ARCHIVE_NAME}" \
		"${OSG_ARCHIVE_REPO_URL}/windows/${OSG_ARCHIVE}.7z" \
		"${OSG_ARCHIVE}.7z"

	if [ -n "$PDBS" ]; then
		download "${OSG_ARCHIVE_NAME} symbols" \
			"${OSG_ARCHIVE_REPO_URL}/windows/${OSG_ARCHIVE}-sym.7z" \
			"${OSG_ARCHIVE}-sym.7z"
	fi

	# SDL2
	download "SDL 2.24.0" \
		"https://gitlab.com/OpenMW/openmw-deps/-/raw/main/windows/SDL2-devel-2.24.0-VC.zip" \
		"SDL2-devel-2.24.0-VC.zip"

	# LZ4
	download "LZ4 ${LZ4_VER}" \
		"https://gitlab.com/OpenMW/openmw-deps/-/raw/main/windows/lz4_win${BITS}_v${LZ4_VER//./_}.7z" \
		"lz4_win${BITS}_v${LZ4_VER//./_}.7z"

	# LuaJIT
	download "LuaJIT ${LUAJIT_VER}" \
		"https://gitlab.com/OpenMW/openmw-deps/-/raw/main/windows/LuaJIT-${LUAJIT_VER}-msvc${LUA_MSVC_YEAR}-win${BITS}.7z" \
		"LuaJIT-${LUAJIT_VER}-msvc${LUA_MSVC_YEAR}-win${BITS}.7z"

	# ICU
	download "ICU ${ICU_VER/_/.}"\
		"https://github.com/unicode-org/icu/releases/download/release-${ICU_VER/_/-}/icu4c-${ICU_VER}-Win${BITS}-MSVC2019.zip" \
		"icu4c-${ICU_VER}-Win${BITS}-MSVC2019.zip"

	download "zlib 1.2.11"\
		"https://gitlab.com/OpenMW/openmw-deps/-/raw/main/windows/zlib-1.2.11-msvc2017-win64.7z" \
		"zlib-1.2.11-msvc2017-win64.7z"
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


if [ -z $APPVEYOR ]; then
	printf "Boost ${BOOST_VER}... "
else
	printf "Boost ${BOOST_VER} AppVeyor... "
fi
{
	if [ -z $APPVEYOR ]; then
		cd $DEPS_INSTALL

		BOOST_SDK="$(real_pwd)/Boost"

		# Boost's installer is still based on ms-dos API that doesn't support larger than 260 char path names
		# We work around this by installing to root of the current working drive and then move it to our deps
		# get the current working drive's root, we'll install to that temporarily
		CWD_DRIVE_ROOT="$(powershell -command '(get-location).Drive.Root')Boost_temp"
		CWD_DRIVE_ROOT_BASH=$(windowsPathAsUnix "$CWD_DRIVE_ROOT")
		if [ -d CWD_DRIVE_ROOT_BASH ]; then
			printf "Cannot continue, ${CWD_DRIVE_ROOT_BASH} aka ${CWD_DRIVE_ROOT} already exists. Please remove before re-running. ";
			wrappedExit 1;
		fi

		if [ -d ${BOOST_SDK} ] && grep "BOOST_VERSION ${BOOST_VER_SDK}" Boost/boost/version.hpp > /dev/null; then
			printf "Exists. "
		elif [ -z $SKIP_EXTRACT ]; then
			rm -rf Boost
			CI_EXTRA_INNO_OPTIONS=""
			[ -n "$CI" ] && CI_EXTRA_INNO_OPTIONS="//SUPPRESSMSGBOXES //LOG='boost_install.log'"
			"${DEPS}/boost-${BOOST_VER}-msvc${MSVC_VER}-win${BITS}.exe" //DIR="${CWD_DRIVE_ROOT}" //VERYSILENT //NORESTART ${CI_EXTRA_INNO_OPTIONS}
			mv "${CWD_DRIVE_ROOT_BASH}" "${BOOST_SDK}"
		fi
		add_cmake_opts -DBOOST_ROOT="$BOOST_SDK" \
			-DBOOST_LIBRARYDIR="${BOOST_SDK}/lib${BITS}-msvc-${MSVC_VER}"
		add_cmake_opts -DBoost_COMPILER="-${TOOLSET}"
		echo Done.
	else
		# Appveyor has all the boost we need already
		BOOST_SDK="c:/Libraries/boost_${BOOST_VER_URL}"

		add_cmake_opts -DBOOST_ROOT="$BOOST_SDK" \
			-DBOOST_LIBRARYDIR="${BOOST_SDK}/lib${BITS}-msvc-${MSVC_VER}.1"
		add_cmake_opts -DBoost_COMPILER="-${TOOLSET}"

		echo Done.
	fi
}
cd $DEPS
echo
printf "Bullet ${BULLET_VER}... "
{
	cd $DEPS_INSTALL
	if [ -d Bullet ]; then
		printf -- "Exists. (No version checking) "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf Bullet
		eval 7z x -y "${DEPS}/Bullet-${BULLET_VER}-msvc${BULLET_MSVC_YEAR}-win${BITS}-double-mt.7z" $STRIP
		mv "Bullet-${BULLET_VER}-msvc${BULLET_MSVC_YEAR}-win${BITS}-double-mt" Bullet
	fi
	add_cmake_opts -DBULLET_ROOT="$(real_pwd)/Bullet"
	echo Done.
}
cd $DEPS
echo
printf "FFmpeg ${FFMPEG_VER}... "
{
	cd $DEPS_INSTALL
	if [ -d FFmpeg ] && grep "${FFMPEG_VER}" FFmpeg/README.txt > /dev/null; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf FFmpeg
		eval 7z x -y "${DEPS}/ffmpeg-${FFMPEG_VER}-win${BITS}.zip" $STRIP
		eval 7z x -y "${DEPS}/ffmpeg-${FFMPEG_VER}-dev-win${BITS}.zip" $STRIP
		mv "ffmpeg-${FFMPEG_VER}-win${BITS}-shared" FFmpeg
		cp -r "ffmpeg-${FFMPEG_VER}-win${BITS}-dev/"* FFmpeg/
		rm -rf "ffmpeg-${FFMPEG_VER}-win${BITS}-dev"
	fi
	export FFMPEG_HOME="$(real_pwd)/FFmpeg"
	for config in ${CONFIGURATIONS[@]}; do
		add_runtime_dlls $config "$(pwd)/FFmpeg/bin/"{avcodec-58,avformat-58,avutil-56,swresample-3,swscale-5}.dll
	done
	if [ $BITS -eq 32 ]; then
		add_cmake_opts "-DCMAKE_EXE_LINKER_FLAGS=\"/machine:X86 /safeseh:no\""
	fi
	echo Done.
}
cd $DEPS
echo
printf "MyGUI 3.4.3... "
{
	cd $DEPS_INSTALL
	if [ -d MyGUI ] && \
		grep "MYGUI_VERSION_MAJOR 3" MyGUI/include/MYGUI/MyGUI_Prerequest.h > /dev/null && \
		grep "MYGUI_VERSION_MINOR 4" MyGUI/include/MYGUI/MyGUI_Prerequest.h > /dev/null && \
		grep "MYGUI_VERSION_PATCH 3" MyGUI/include/MYGUI/MyGUI_Prerequest.h > /dev/null
	then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf MyGUI
		eval 7z x -y "${DEPS}/MyGUI-3.4.3-msvc${MYGUI_MSVC_YEAR}-win${BITS}.7z" $STRIP
		[ -n "$PDBS" ] && eval 7z x -y "${DEPS}/MyGUI-3.4.3-msvc${MYGUI_MSVC_YEAR}-win${BITS}-sym.7z" $STRIP
		mv "MyGUI-3.4.3-msvc${MYGUI_MSVC_YEAR}-win${BITS}" MyGUI
	fi
	export MYGUI_HOME="$(real_pwd)/MyGUI"
	for CONFIGURATION in ${CONFIGURATIONS[@]}; do
		if [ $CONFIGURATION == "Debug" ]; then
			SUFFIX="_d"
			MYGUI_CONFIGURATION="Debug"
		else
			SUFFIX=""
			MYGUI_CONFIGURATION="RelWithDebInfo"
		fi
		add_runtime_dlls $CONFIGURATION "$(pwd)/MyGUI/bin/${MYGUI_CONFIGURATION}/MyGUIEngine${SUFFIX}.dll"
	done
	echo Done.
}
cd $DEPS
echo
printf "OpenAL-Soft ${OPENAL_VER}... "
{
	if [ -d openal-soft-${OPENAL_VER}-bin ]; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf openal-soft-${OPENAL_VER}-bin
		eval 7z x -y OpenAL-Soft-${OPENAL_VER}.zip $STRIP
	fi
	OPENAL_SDK="$(real_pwd)/openal-soft-${OPENAL_VER}-bin"
	add_cmake_opts -DOPENAL_INCLUDE_DIR="${OPENAL_SDK}/include/AL" \
		-DOPENAL_LIBRARY="${OPENAL_SDK}/libs/Win${BITS}/OpenAL32.lib"
	for config in ${CONFIGURATIONS[@]}; do
		add_runtime_dlls $config "$(pwd)/openal-soft-${OPENAL_VER}-bin/bin/WIN${BITS}/soft_oal.dll:OpenAL32.dll"
	done
	echo Done.
}
cd $DEPS
echo
printf "${OSG_ARCHIVE_NAME}... "
{
	cd $DEPS_INSTALL
	if [ -d OSG ] && \
		grep "OPENSCENEGRAPH_MAJOR_VERSION    3" OSG/include/osg/Version > /dev/null && \
		grep "OPENSCENEGRAPH_MINOR_VERSION    6" OSG/include/osg/Version > /dev/null && \
		grep "OPENSCENEGRAPH_PATCH_VERSION    5" OSG/include/osg/Version > /dev/null
	then
	printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf OSG
		eval 7z x -y "${DEPS}/${OSG_ARCHIVE}.7z" $STRIP
		[ -n "$PDBS" ] && eval 7z x -y "${DEPS}/${OSG_ARCHIVE}-sym.7z" $STRIP
		mv "${OSG_ARCHIVE}" OSG
	fi
	OSG_SDK="$(real_pwd)/OSG"
	add_cmake_opts -DOSG_DIR="$OSG_SDK"
	for CONFIGURATION in ${CONFIGURATIONS[@]}; do
		if [ $CONFIGURATION == "Debug" ]; then
			SUFFIX="d"
			SUFFIX_UPCASE="D"
		else
			SUFFIX=""
			SUFFIX_UPCASE=""
		fi

		if [[ -n "$OSG_MULTIVIEW_BUILD" ]]; then
			add_runtime_dlls $CONFIGURATION "$(pwd)/OSG/bin/"{ot21-OpenThreads,libpng16}${SUFFIX}.dll \
				"$(pwd)/OSG/bin/osg162-osg"{,Animation,DB,FX,GA,Particle,Text,Util,Viewer,Shadow,Sim}${SUFFIX}.dll
		else
			add_runtime_dlls $CONFIGURATION "$(pwd)/OSG/bin/"{OpenThreads,icuuc58,libpng16}${SUFFIX}.dll \
				"$(pwd)/OSG/bin/libxml2"${SUFFIX_UPCASE}.dll \
				"$(pwd)/OSG/bin/osg"{,Animation,DB,FX,GA,Particle,Text,Util,Viewer,Shadow,Sim}${SUFFIX}.dll
			add_runtime_dlls $CONFIGURATION "$(pwd)/OSG/bin/icudt58.dll"
			if [ $CONFIGURATION == "Debug" ]; then
				add_runtime_dlls $CONFIGURATION "$(pwd)/OSG/bin/"{boost_filesystem-vc141-mt-gd-1_63,boost_system-vc141-mt-gd-1_63,collada-dom2.4-dp-vc141-mt-d}.dll
			else
				add_runtime_dlls $CONFIGURATION "$(pwd)/OSG/bin/"{boost_filesystem-vc141-mt-gd-1_63,boost_system-vc141-mt-1_63,collada-dom2.4-dp-vc141-mt}.dll
			fi
		fi
		add_osg_dlls $CONFIGURATION "$(pwd)/OSG/bin/osgPlugins-3.6.5/osgdb_"{bmp,dae,dds,freetype,jpeg,osg,png,tga}${SUFFIX}.dll
		add_osg_dlls $CONFIGURATION "$(pwd)/OSG/bin/osgPlugins-3.6.5/osgdb_serializers_osg"{,animation,fx,ga,particle,text,util,viewer,shadow}${SUFFIX}.dll
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
		AQT_VERSION="v3.1.12"
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
cd $DEPS
echo
printf "SDL 2.24.0... "
{
	if [ -d SDL2-2.24.0 ]; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf SDL2-2.24.0
		eval 7z x -y SDL2-devel-2.24.0-VC.zip $STRIP
	fi
	SDL2DIR="$(real_pwd)/SDL2-2.24.0"
	for config in ${CONFIGURATIONS[@]}; do
		add_runtime_dlls $config "$(pwd)/SDL2-2.24.0/lib/x${ARCHSUFFIX}/SDL2.dll"
	done
	echo Done.
}
cd $DEPS
echo
printf "LZ4 ${LZ4_VER}... "
{
	if [ -d LZ4_${LZ4_VER} ]; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf LZ4_${LZ4_VER}
		eval 7z x -y lz4_win${BITS}_v${LZ4_VER//./_}.7z -o$(real_pwd)/LZ4_${LZ4_VER} $STRIP
	fi
	export LZ4DIR="$(real_pwd)/LZ4_${LZ4_VER}"
	add_cmake_opts -DLZ4_INCLUDE_DIR="${LZ4DIR}/include" \
		-DLZ4_LIBRARY="${LZ4DIR}/lib/liblz4.lib"
	for CONFIGURATION in ${CONFIGURATIONS[@]}; do
		if [ $CONFIGURATION == "Debug" ]; then
			LZ4_CONFIGURATION="Debug"
		else
			SUFFIX=""
			LZ4_CONFIGURATION="Release"
		fi
		add_runtime_dlls $CONFIGURATION "$(pwd)/LZ4_${LZ4_VER}/bin/${LZ4_CONFIGURATION}/liblz4.dll"
	done
	echo Done.
}
cd $DEPS
echo
printf "LuaJIT ${LUAJIT_VER}... "
{
	if [ -d LuaJIT ]; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf LuaJIT
		eval 7z x -y LuaJIT-${LUAJIT_VER}-msvc${LUA_MSVC_YEAR}-win${BITS}.7z -o$(real_pwd)/LuaJIT $STRIP
	fi
	export LUAJIT_DIR="$(real_pwd)/LuaJIT"
	add_cmake_opts -DLuaJit_INCLUDE_DIR="${LUAJIT_DIR}/include" \
		-DLuaJit_LIBRARY="${LUAJIT_DIR}/lib/lua51.lib"
	for CONFIGURATION in ${CONFIGURATIONS[@]}; do
		add_runtime_dlls $CONFIGURATION "$(pwd)/LuaJIT/bin/lua51.dll"
	done
	echo Done.
}

cd $DEPS
echo
printf "ICU ${ICU_VER/_/.}... "
{
	if [ -d ICU-${ICU_VER} ]; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf ICU-${ICU_VER}
		eval 7z x -y icu4c-${ICU_VER}-Win${BITS}-MSVC2019.zip -o$(real_pwd)/ICU-${ICU_VER} $STRIP
	fi
	ICU_ROOT="$(real_pwd)/ICU-${ICU_VER}"
	add_cmake_opts -DICU_ROOT="${ICU_ROOT}" \
		-DICU_INCLUDE_DIR="${ICU_ROOT}/include" \
		-DICU_I18N_LIBRARY="${ICU_ROOT}/lib${BITS}/icuin.lib " \
		-DICU_UC_LIBRARY="${ICU_ROOT}/lib${BITS}/icuuc.lib " \
		-DICU_DEBUG=ON

	for config in ${CONFIGURATIONS[@]}; do
		add_runtime_dlls $config "$(pwd)/ICU-${ICU_VER}/bin${BITS}/icudt${ICU_VER/_*/}.dll"
		add_runtime_dlls $config "$(pwd)/ICU-${ICU_VER}/bin${BITS}/icuin${ICU_VER/_*/}.dll"
		add_runtime_dlls $config "$(pwd)/ICU-${ICU_VER}/bin${BITS}/icuuc${ICU_VER/_*/}.dll"
	done
	echo Done.
}

cd $DEPS
echo
printf "zlib 1.2.11... "
{
	if [ -d zlib-1.2.11-msvc2017-win64 ]; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf zlib-1.2.11-msvc2017-win64
		eval 7z x -y zlib-1.2.11-msvc2017-win64.7z $STRIP
	fi
	add_cmake_opts -DZLIB_ROOT="$(real_pwd)/zlib-1.2.11-msvc2017-win64"
	for config in ${CONFIGURATIONS[@]}; do
		if [ $config == "Debug" ]; then
			add_runtime_dlls $config "$(pwd)/zlib-1.2.11-msvc2017-win64/bin/zlibd.dll"
		else
			add_runtime_dlls $config "$(pwd)/zlib-1.2.11-msvc2017-win64/bin/zlib.dll"
		fi
	done
	echo Done.
}

add_cmake_opts -DCMAKE_PREFIX_PATH="\"${QT_SDK};${SDL2DIR}\""

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
