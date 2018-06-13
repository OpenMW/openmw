#!/bin/bash

MISSINGTOOLS=0

command -v 7z >/dev/null 2>&1 || { echo "Error: 7z (7zip) is not on the path."; MISSINGTOOLS=1; }
command -v cmake >/dev/null 2>&1 || { echo "Error: cmake (CMake) is not on the path."; MISSINGTOOLS=1; }

if [ $MISSINGTOOLS -ne 0 ]; then
	exit 1
fi

WORKINGDIR="$(pwd)"
case "$WORKINGDIR" in
	*[[:space:]]*)
		echo "Error: Working directory contains spaces."
		exit 1
		;;
esac

set -euo pipefail

APPVEYOR=${APPVEYOR:-}
CI=${CI:-}
STEP=${STEP:-}

VERBOSE=""
STRIP=""
SKIP_DOWNLOAD=""
SKIP_EXTRACT=""
KEEP=""
UNITY_BUILD=""
VS_VERSION=""
PLATFORM=""
CONFIGURATION=""

while [ $# -gt 0 ]; do
	ARGSTR=$1
	shift

	if [ ${ARGSTR:0:1} != "-" ]; then
		echo "Unknown argument $ARGSTR"
		echo "Try '$0 -h'"
		exit 1
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

			k )
				KEEP=true ;;

			u )
				UNITY_BUILD=true ;;

			v )
				VS_VERSION=$1
				shift ;;

			p )
				PLATFORM=$1
				shift ;;

			c )
				CONFIGURATION=$1
				shift ;;

			h )
				cat <<EOF
Usage: $0 [-cdehkpuvV]

Options:
	-c <Release/Debug>
		Set the configuration, can also be set with environment variable CONFIGURATION.
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
	-u
		Configure for unity builds.
	-v <2013/2015/2017>
		Choose the Visual Studio version to use.
	-V
		Run verbosely
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

if [ -z $VERBOSE ]; then
	STRIP="> /dev/null 2>&1"
fi

if [ -z $APPVEYOR ]; then
	echo "Running prebuild outside of Appveyor."

	DIR=$(echo "$0" | sed "s,\\\\,/,g" | sed "s,\(.\):,/\\1,")
	cd $(dirname "$DIR")/..
else
	echo "Running prebuild in Appveyor."

	cd "$APPVEYOR_BUILD_FOLDER"
fi

run_cmd() {
	CMD="$1"
	shift

	if [ -z $VERBOSE ]; then
		eval $CMD $@ > output.log 2>&1
		RET=$?

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
		eval $CMD $@
		return $?
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
				curl --silent --retry 10 -kLy 5 -o $FILE $URL
				RET=$?
			else
				curl --retry 10 -kLy 5 -o $FILE $URL
				RET=$?
			fi

			if [ $RET -ne 0 ]; then
				echo "Failed!"
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
	pwd | sed "s,/\(.\),\1:,"
}

CMAKE_OPTS=""
add_cmake_opts() {
	CMAKE_OPTS="$CMAKE_OPTS $@"
}

RUNTIME_DLLS=""
add_runtime_dlls() {
	RUNTIME_DLLS="$RUNTIME_DLLS $@"
}

OSG_PLUGINS=""
add_osg_dlls() {
	OSG_PLUGINS="$OSG_PLUGINS $@"
}

QT_PLATFORMS=""
add_qt_platform_dlls() {
	QT_PLATFORMS="$QT_PLATFORMS $@"
}

if [ -z $PLATFORM ]; then
	PLATFORM="$(uname -m)"
fi

if [ -z $CONFIGURATION ]; then
	CONFIGURATION="Debug"
fi

if [ -z $VS_VERSION ]; then
	VS_VERSION="2013"
fi

case $VS_VERSION in
	15|15.0|2017 )
		GENERATOR="Visual Studio 15 2017"
		TOOLSET="vc140"
		TOOLSET_REAL="vc141"
		MSVC_REAL_VER="15"
		MSVC_VER="14"
		MSVC_YEAR="2015"
		MSVC_DISPLAY_YEAR="2017"
		;;

	14|14.0|2015 )
		GENERATOR="Visual Studio 14 2015"
		TOOLSET="vc140"
		TOOLSET_REAL="vc140"
		MSVC_REAL_VER="14"
		MSVC_VER="14"
		MSVC_YEAR="2015"
		MSVC_DISPLAY_YEAR="2015"
		;;

	12|12.0|2013 )
		GENERATOR="Visual Studio 12 2013"
		TOOLSET="vc120"
		TOOLSET_REAL="vc120"
		MSVC_REAL_VER="12"
		MSVC_VER="12"
		MSVC_YEAR="2013"
		MSVC_DISPLAY_YEAR="2013"
		;;
esac

case $PLATFORM in
	x64|x86_64|x86-64|win64|Win64 )
		ARCHNAME="x86-64"
		ARCHSUFFIX="64"
		BITS="64"

		BASE_OPTS="-G\"$GENERATOR Win64\""
		add_cmake_opts "-G\"$GENERATOR Win64\""
		;;

	x32|x86|i686|i386|win32|Win32 )
		ARCHNAME="x86"
		ARCHSUFFIX="86"
		BITS="32"

		BASE_OPTS="-G\"$GENERATOR\""
		add_cmake_opts "-G\"$GENERATOR\""
		;;

	* )
		echo "Unknown platform $PLATFORM."
		exit 1
		;;
esac

case $CONFIGURATION in
	debug|Debug|DEBUG )
		CONFIGURATION=Debug
		BUILD_CONFIG=Debug
		;;

	release|Release|RELEASE )
		CONFIGURATION=Release
		BUILD_CONFIG=Release
		;;

	relwithdebinfo|RelWithDebInfo|RELWITHDEBINFO )
		CONFIGURATION=Release
		BUILD_CONFIG=RelWithDebInfo
		;;
esac

if ! [ -z $UNITY_BUILD ]; then
	add_cmake_opts "-DOPENMW_UNITY_BUILD=True"
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
		download "Boost 1.61.0" \
			"https://sourceforge.net/projects/boost/files/boost-binaries/1.61.0/boost_1_61_0-msvc-${MSVC_VER}.0-${BITS}.exe" \
			"boost-1.61.0-msvc${MSVC_YEAR}-win${BITS}.exe"
	fi

	# Bullet
	download "Bullet 2.86" \
		"https://www.lysator.liu.se/~ace/OpenMW/deps/Bullet-2.86-msvc${MSVC_YEAR}-win${BITS}.7z" \
		"Bullet-2.86-msvc${MSVC_YEAR}-win${BITS}.7z"

	# FFmpeg
	download "FFmpeg 3.2.4" \
		"https://ffmpeg.zeranoe.com/builds/win${BITS}/shared/ffmpeg-3.2.4-win${BITS}-shared.zip" \
		"ffmpeg-3.2.4-win${BITS}.zip" \
		"https://ffmpeg.zeranoe.com/builds/win${BITS}/dev/ffmpeg-3.2.4-win${BITS}-dev.zip" \
		"ffmpeg-3.2.4-dev-win${BITS}.zip"

	# MyGUI
	download "MyGUI 3.2.2" \
		"https://www.lysator.liu.se/~ace/OpenMW/deps/MyGUI-3.2.2-msvc${MSVC_YEAR}-win${BITS}.7z" \
		"MyGUI-3.2.2-msvc${MSVC_YEAR}-win${BITS}.7z"

	# OpenAL
	download "OpenAL-Soft 1.17.2" \
		"http://kcat.strangesoft.net/openal-binaries/openal-soft-1.17.2-bin.zip" \
		"OpenAL-Soft-1.17.2.zip"

	# OSG
	download "OpenSceneGraph 3.4.1-scrawl" \
		"https://www.lysator.liu.se/~ace/OpenMW/deps/OSG-3.4.1-scrawl-msvc${MSVC_YEAR}-win${BITS}.7z" \
		"OSG-3.4.1-scrawl-msvc${MSVC_YEAR}-win${BITS}.7z"

	# Qt
	if [ -z $APPVEYOR ]; then
		if [ $BITS == "64" ]; then
			QT_SUFFIX="_64"
		else
			QT_SUFFIX=""
		fi

		download "Qt 5.7.2" \
			"https://download.qt.io/official_releases/qt/5.7/5.7.0/qt-opensource-windows-x86-msvc${MSVC_YEAR}${QT_SUFFIX}-5.7.0.exe" \
			"qt-5.7.0-msvc${MSVC_YEAR}-win${BITS}.exe" \
			"https://www.lysator.liu.se/~ace/OpenMW/deps/qt-5-install.qs" \
			"qt-5-install.qs"
	fi

	# SDL2
	download "SDL 2.0.7" \
		"https://www.libsdl.org/release/SDL2-devel-2.0.7-VC.zip" \
		"SDL2-2.0.7.zip"
fi

cd .. #/..

# Set up dependencies
BUILD_DIR="MSVC${MSVC_DISPLAY_YEAR}_${BITS}"
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


# Boost
if [ -z $APPVEYOR ]; then
	printf "Boost 1.61.0... "
else
	if [ $MSVC_VER -eq 12 ]; then
		printf "Boost 1.58.0 AppVeyor... "
	else
		printf "Boost 1.67.0 AppVeyor... "
	fi
fi
{
	if [ -z $APPVEYOR ]; then
		cd $DEPS_INSTALL

		BOOST_SDK="$(real_pwd)/Boost"

		if [ -d Boost ] && grep "BOOST_VERSION 106100" Boost/boost/version.hpp > /dev/null; then
			printf "Exists. "
		elif [ -z $SKIP_EXTRACT ]; then
			rm -rf Boost
			"${DEPS}/boost-1.61.0-msvc${MSVC_YEAR}-win${BITS}.exe" //dir="$(echo $BOOST_SDK | sed s,/,\\\\,g)" //verysilent
		fi

		add_cmake_opts -DBOOST_ROOT="$BOOST_SDK" \
			-DBOOST_LIBRARYDIR="${BOOST_SDK}/lib${BITS}-msvc-${MSVC_VER}.0"
		add_cmake_opts -DBoost_COMPILER="-${TOOLSET}"

		echo Done.
	else
		# Appveyor unstable has all the boost we need already
		if [ $MSVC_REAL_VER -eq 12 ]; then
			BOOST_SDK="c:/Libraries/boost_1_58_0"
		else
			BOOST_SDK="c:/Libraries/boost_1_67_0"
		fi
		if [ $MSVC_REAL_VER -eq 15 ]; then
			LIB_SUFFIX="1"
		else
			LIB_SUFFIX="0"
		fi
		
		add_cmake_opts -DBOOST_ROOT="$BOOST_SDK" \
			-DBOOST_LIBRARYDIR="${BOOST_SDK}/lib${BITS}-msvc-${MSVC_VER}.${LIB_SUFFIX}"
		add_cmake_opts -DBoost_COMPILER="-${TOOLSET_REAL}"

		echo Done.
	fi
}
cd $DEPS
echo

# Bullet
printf "Bullet 2.86... "
{
	cd $DEPS_INSTALL

	if [ -d Bullet ]; then
		printf -- "Exists. (No version checking) "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf Bullet
		eval 7z x -y "${DEPS}/Bullet-2.86-msvc${MSVC_YEAR}-win${BITS}.7z" $STRIP
		mv "Bullet-2.86-msvc${MSVC_YEAR}-win${BITS}" Bullet
	fi

	export BULLET_ROOT="$(real_pwd)/Bullet"

	echo Done.
}
cd $DEPS
echo

# FFmpeg
printf "FFmpeg 3.2.4... "
{
	cd $DEPS_INSTALL

	if [ -d FFmpeg ] && grep "FFmpeg version: 3.2.4" FFmpeg/README.txt > /dev/null; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf FFmpeg

		eval 7z x -y "${DEPS}/ffmpeg-3.2.4-win${BITS}.zip" $STRIP
		eval 7z x -y "${DEPS}/ffmpeg-3.2.4-dev-win${BITS}.zip" $STRIP

		mv "ffmpeg-3.2.4-win${BITS}-shared" FFmpeg
		cp -r "ffmpeg-3.2.4-win${BITS}-dev/"* FFmpeg/
		rm -rf "ffmpeg-3.2.4-win${BITS}-dev"
	fi

	export FFMPEG_HOME="$(real_pwd)/FFmpeg"
	add_runtime_dlls "$(pwd)/FFmpeg/bin/"{avcodec-57,avformat-57,avutil-55,swresample-2,swscale-4}.dll

	if [ $BITS -eq 32 ]; then
		add_cmake_opts "-DCMAKE_EXE_LINKER_FLAGS=\"/machine:X86 /safeseh:no\""
	fi

	echo Done.
}
cd $DEPS
echo

# MyGUI
printf "MyGUI 3.2.2... "
{
	cd $DEPS_INSTALL

	if [ -d MyGUI ] && \
		grep "MYGUI_VERSION_MAJOR 3" MyGUI/include/MYGUI/MyGUI_Prerequest.h > /dev/null && \
		grep "MYGUI_VERSION_MINOR 2" MyGUI/include/MYGUI/MyGUI_Prerequest.h > /dev/null && \
		grep "MYGUI_VERSION_PATCH 2" MyGUI/include/MYGUI/MyGUI_Prerequest.h > /dev/null
	then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf MyGUI
		eval 7z x -y "${DEPS}/MyGUI-3.2.2-msvc${MSVC_YEAR}-win${BITS}.7z" $STRIP
		mv "MyGUI-3.2.2-msvc${MSVC_YEAR}-win${BITS}" MyGUI
	fi

	export MYGUI_HOME="$(real_pwd)/MyGUI"

	if [ $CONFIGURATION == "Debug" ]; then
		SUFFIX="_d"
	else
		SUFFIX=""
	fi
	add_runtime_dlls "$(pwd)/MyGUI/bin/${CONFIGURATION}/MyGUIEngine${SUFFIX}.dll"

	echo Done.
}
cd $DEPS
echo

# OpenAL
printf "OpenAL-Soft 1.17.2... "
{
	if [ -d openal-soft-1.17.2-bin ]; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf openal-soft-1.17.2-bin
		eval 7z x -y OpenAL-Soft-1.17.2.zip $STRIP
	fi

	OPENAL_SDK="$(real_pwd)/openal-soft-1.17.2-bin"

	add_cmake_opts -DOPENAL_INCLUDE_DIR="${OPENAL_SDK}/include/AL" \
		-DOPENAL_LIBRARY="${OPENAL_SDK}/libs/Win${BITS}/OpenAL32.lib"

	add_runtime_dlls "$(pwd)/openal-soft-1.17.2-bin/bin/WIN${BITS}/soft_oal.dll:OpenAL32.dll"

	echo Done.
}
cd $DEPS
echo

# OSG
printf "OSG 3.4.1-scrawl... "
{
	cd $DEPS_INSTALL

	if [ -d OSG ] && \
		grep "OPENSCENEGRAPH_MAJOR_VERSION    3" OSG/include/osg/Version > /dev/null && \
		grep "OPENSCENEGRAPH_MINOR_VERSION    4" OSG/include/osg/Version > /dev/null && \
		grep "OPENSCENEGRAPH_PATCH_VERSION    1" OSG/include/osg/Version > /dev/null
	then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf OSG
		eval 7z x -y "${DEPS}/OSG-3.4.1-scrawl-msvc${MSVC_YEAR}-win${BITS}.7z" $STRIP
		mv "OSG-3.4.1-scrawl-msvc${MSVC_YEAR}-win${BITS}" OSG
	fi

	OSG_SDK="$(real_pwd)/OSG"

	add_cmake_opts -DOSG_DIR="$OSG_SDK"

	if [ $CONFIGURATION == "Debug" ]; then
		SUFFIX="d"
	else
		SUFFIX=""
	fi

	add_runtime_dlls "$(pwd)/OSG/bin/"{OpenThreads,zlib,libpng*}${SUFFIX}.dll \
		"$(pwd)/OSG/bin/osg"{,Animation,DB,FX,GA,Particle,Text,Util,Viewer}${SUFFIX}.dll

	add_osg_dlls "$(pwd)/OSG/bin/osgPlugins-3.4.1/osgdb_"{bmp,dds,jpeg,osg,png,tga}${SUFFIX}.dll
	add_osg_dlls "$(pwd)/OSG/bin/osgPlugins-3.4.1/osgdb_serializers_osg"{,animation,fx,ga,particle,text,util,viewer}${SUFFIX}.dll

	echo Done.
}
cd $DEPS
echo

# Qt
if [ -z $APPVEYOR ]; then
	printf "Qt 5.7.0... "
else
	printf "Qt 5.10 AppVeyor... "
fi
{
	if [ $BITS -eq 64 ]; then
		SUFFIX="_64"
	else
		SUFFIX=""
	fi

	if [ -z $APPVEYOR ]; then
		cd $DEPS_INSTALL
		QT_SDK="$(real_pwd)/Qt/5.7/msvc${MSVC_YEAR}${SUFFIX}"

		if [ -d Qt ] && head -n2 Qt/InstallationLog.txt | grep "5.7.0" > /dev/null; then
			printf "Exists. "
		elif [ -z $SKIP_EXTRACT ]; then
			rm -rf Qt
			cp "${DEPS}/qt-5-install.qs" qt-install.qs


			sed -i "s|INSTALL_DIR|$(real_pwd)/Qt|" qt-install.qs
			sed -i "s/qt.VERSION.winBITS_msvcYEAR/qt.57.win${BITS}_msvc${MSVC_YEAR}${SUFFIX}/" qt-install.qs

			printf -- "(Installation might take a while) "
			"${DEPS}/qt-5.7.0-msvc${MSVC_YEAR}-win${BITS}.exe" --script qt-install.qs --silent

			mv qt-install.qs Qt/

			echo Done.
			printf "  Cleaning up extraneous data... "
			rm -r "$(real_pwd)/Qt/"{dist,Docs,Examples,Tools,vcredist,components.xml,MaintenanceTool.dat,MaintenanceTool.exe,MaintenanceTool.ini,network.xml,qt-install.qs}
		fi

		cd $QT_SDK

		add_cmake_opts -DDESIRED_QT_VERSION=5 \
			-DQT_QMAKE_EXECUTABLE="${QT_SDK}/bin/qmake.exe" \
			-DCMAKE_PREFIX_PATH="$QT_SDK"

		if [ $CONFIGURATION == "Debug" ]; then
			SUFFIX="d"
		else
			SUFFIX=""
		fi

		add_runtime_dlls "$(pwd)/bin/Qt5"{Core,Gui,Network,OpenGL,Widgets}${SUFFIX}.dll
		add_qt_platform_dlls "$(pwd)/plugins/platforms/qwindows${SUFFIX}.dll"

		echo Done.
	else
		QT_SDK="C:/Qt/5.10/msvc${MSVC_DISPLAY_YEAR}${SUFFIX}"

		add_cmake_opts -DDESIRED_QT_VERSION=5 \
			-DQT_QMAKE_EXECUTABLE="${QT_SDK}/bin/qmake.exe" \
			-DCMAKE_PREFIX_PATH="$QT_SDK"

		if [ $CONFIGURATION == "Debug" ]; then
			SUFFIX="d"
		else
			SUFFIX=""
		fi

		DIR=$(echo "${QT_SDK}" | sed "s,\\\\,/,g" | sed "s,\(.\):,/\\1,")

		add_runtime_dlls "${DIR}/bin/Qt5"{Core,Gui,Network,OpenGL,Widgets}${SUFFIX}.dll
		add_qt_platform_dlls "${DIR}/plugins/platforms/qwindows${SUFFIX}.dll"

		echo Done.
	fi
}
cd $DEPS
echo

# SDL2
printf "SDL 2.0.7... "
{
	if [ -d SDL2-2.0.7 ]; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf SDL2-2.0.7
		eval 7z x -y SDL2-2.0.7.zip $STRIP
	fi

	export SDL2DIR="$(real_pwd)/SDL2-2.0.7"

	add_runtime_dlls "$(pwd)/SDL2-2.0.7/lib/x${ARCHSUFFIX}/SDL2.dll"

	echo Done.
}
echo


cd $DEPS_INSTALL/..

echo
echo "Setting up OpenMW build..."

add_cmake_opts -DBUILD_BSATOOL=no \
	-DBUILD_ESMTOOL=no \
	-DBUILD_MYGUI_PLUGIN=no \
	-DOPENMW_MP_BUILD=on

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
	echo "- Copying Runtime DLLs..."
	mkdir -p $BUILD_CONFIG
	for DLL in $RUNTIME_DLLS; do
		TARGET="$(basename "$DLL")"
		if [[ "$DLL" == *":"* ]]; then
			IFS=':'; SPLIT=( ${DLL} ); unset IFS

			DLL=${SPLIT[0]}
			TARGET=${SPLIT[1]}
		fi

		echo "    ${TARGET}."
		cp "$DLL" "$BUILD_CONFIG/$TARGET"
	done
	echo

	echo "- OSG Plugin DLLs..."
	mkdir -p $BUILD_CONFIG/osgPlugins-3.4.1
	for DLL in $OSG_PLUGINS; do
		echo "    $(basename $DLL)."
		cp "$DLL" $BUILD_CONFIG/osgPlugins-3.4.1
	done
	echo

	echo "- Qt Platform DLLs..."
	mkdir -p ${BUILD_CONFIG}/platforms
	for DLL in $QT_PLATFORMS; do
		echo "    $(basename $DLL)"
		cp "$DLL" "${BUILD_CONFIG}/platforms"
	done
	echo
#fi

if [ -z $VERBOSE ]; then
	printf -- "- Configuring... "
else
	echo "- cmake .. $CMAKE_OPTS"
fi

run_cmd cmake .. $CMAKE_OPTS
RET=$?

if [ -z $VERBOSE ]; then
	if [ $RET -eq 0 ]; then
		echo Done.
	else
		echo Failed.
	fi
fi

exit $RET
