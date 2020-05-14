#!/bin/bash
# set -x  # turn-on for debugging

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
NMAKE=""
PDBS=""
PLATFORM=""
CONFIGURATION=""
TEST_FRAMEWORK=""
GOOGLE_INSTALL_ROOT=""

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

			n )
				NMAKE=true ;;

			p )
				PLATFORM=$1
				shift ;;

			P )
				PDBS=true ;;

			c )
				CONFIGURATION=$1
				shift ;;

			t )
				TEST_FRAMEWORK=true ;;

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
	-t
		Build unit tests / Google test
	-u
		Configure for unity builds.
	-v <2013/2015/2017/2019>
		Choose the Visual Studio version to use.
	-n
		Produce NMake makefiles instead of a Visual Studio solution.
	-P
		Download debug symbols where available
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

if [ -n "$NMAKE" ]; then
	command -v nmake -? >/dev/null 2>&1 || { echo "Error: nmake (NMake) is not on the path. Make sure you have the necessary environment variables set for command-line C++ development (for example, by starting from a Developer Command Prompt)."; exit 1; }
fi

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
	VS_VERSION="2017"
fi

case $VS_VERSION in
	16|16.0|2019 )
		GENERATOR="Visual Studio 16 2019"
		TOOLSET="vc142"
		MSVC_REAL_VER="16"
		MSVC_VER="14.2"
		MSVC_YEAR="2015"
		MSVC_REAL_YEAR="2019"
		MSVC_DISPLAY_YEAR="2019"
		BOOST_VER="1.71.0"
		BOOST_VER_URL="1_71_0"
		BOOST_VER_SDK="107100"
		;;

	15|15.0|2017 )
		GENERATOR="Visual Studio 15 2017"
		TOOLSET="vc141"
		MSVC_REAL_VER="15"
		MSVC_VER="14.1"
		MSVC_YEAR="2015"
		MSVC_REAL_YEAR="2017"
		MSVC_DISPLAY_YEAR="2017"
		BOOST_VER="1.67.0"
		BOOST_VER_URL="1_67_0"
		BOOST_VER_SDK="106700"
		;;

	14|14.0|2015 )
		GENERATOR="Visual Studio 14 2015"
		TOOLSET="vc140"
		MSVC_REAL_VER="14"
		MSVC_VER="14.0"
		MSVC_YEAR="2015"
		MSVC_REAL_YEAR="2015"
		MSVC_DISPLAY_YEAR="2015"
		BOOST_VER="1.67.0"
		BOOST_VER_URL="1_67_0"
		BOOST_VER_SDK="106700"
		;;

	12|12.0|2013 )
		echo "Visual Studio 2013 is no longer supported"
		exit 1
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

if [ $BITS -eq 64 ] && [ $MSVC_REAL_VER -lt 16 ]; then
	GENERATOR="${GENERATOR} Win64"
fi

if [ -n "$NMAKE" ]; then
	GENERATOR="NMake Makefiles"
fi

if [ $MSVC_REAL_VER -ge 16 ]; then
	if [ $BITS -eq 64 ]; then
		add_cmake_opts "-G\"$GENERATOR\" -A x64"
	else
		add_cmake_opts "-G\"$GENERATOR\" -A Win32"
	fi
else
	add_cmake_opts "-G\"$GENERATOR\""
fi

if [ -n "$NMAKE" ]; then
	add_cmake_opts "-DCMAKE_BUILD_TYPE=${BUILD_CONFIG}"
fi

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
		download "Boost ${BOOST_VER}" \
			"https://sourceforge.net/projects/boost/files/boost-binaries/${BOOST_VER}/boost_${BOOST_VER_URL}-msvc-${MSVC_VER}-${BITS}.exe" \
			"boost-${BOOST_VER}-msvc${MSVC_VER}-win${BITS}.exe"
	fi

	# Bullet
	download "Bullet 2.86" \
		"https://rgw.ctrl-c.liu.se/openmw/Deps/Bullet-2.86-msvc${MSVC_YEAR}-win${BITS}.7z" \
		"Bullet-2.86-msvc${MSVC_YEAR}-win${BITS}.7z"

	# FFmpeg
	download "FFmpeg 3.2.4" \
		"https://ffmpeg.zeranoe.com/builds/win${BITS}/shared/ffmpeg-3.2.4-win${BITS}-shared.zip" \
		"ffmpeg-3.2.4-win${BITS}.zip" \
		"https://ffmpeg.zeranoe.com/builds/win${BITS}/dev/ffmpeg-3.2.4-win${BITS}-dev.zip" \
		"ffmpeg-3.2.4-dev-win${BITS}.zip"

	# MyGUI
	download "MyGUI 3.2.2" \
		"https://rgw.ctrl-c.liu.se/openmw/Deps/MyGUI-3.2.2-msvc${MSVC_YEAR}-win${BITS}.7z" \
		"MyGUI-3.2.2-msvc${MSVC_YEAR}-win${BITS}.7z"

	# OpenAL
	download "OpenAL-Soft 1.19.1" \
		"http://openal-soft.org/openal-binaries/openal-soft-1.19.1-bin.zip" \
		"OpenAL-Soft-1.19.1.zip"

	# OSG
	download "OpenSceneGraph 3.4.2-experimental" \
		"https://rgw.ctrl-c.liu.se/openmw/Deps/OSG-3.4.2-experimental-msvc${MSVC_REAL_YEAR}-win${BITS}.7z" \
		"OSG-3.4.2-experimental-msvc${MSVC_REAL_YEAR}-win${BITS}.7z"

	if [ -n "$PDBS" ]; then
		download "OpenSceneGraph symbols" \
			"https://rgw.ctrl-c.liu.se/openmw/Deps/OSG-3.4.2-experimental-msvc${MSVC_REAL_YEAR}-win${BITS}-sym.7z" \
			"OSG-3.4.2-experimental-msvc${MSVC_REAL_YEAR}-win${BITS}-sym.7z"
	fi

	# Qt
	if [ -z $APPVEYOR ]; then
		if [ $BITS == "64" ]; then
			QT_SUFFIX="_64"
		else
			QT_SUFFIX=""
		fi

		download "Qt 5.7.0" \
			"https://download.qt.io/new_archive/qt/5.7/5.7.0/qt-opensource-windows-x86-msvc${MSVC_YEAR}${QT_SUFFIX}-5.7.0.exe" \
			"qt-5.7.0-msvc${MSVC_YEAR}-win${BITS}.exe" \
			"https://www.lysator.liu.se/~ace/OpenMW/deps/qt-5-install.qs" \
			"qt-5-install.qs"
	fi

	# SDL2
	download "SDL 2.0.12" \
		"https://www.libsdl.org/release/SDL2-devel-2.0.12-VC.zip" \
		"SDL2-2.0.12.zip"

	# Google test and mock
	if [ ! -z $TEST_FRAMEWORK ]; then
		echo "Google test 1.8.1..."
		if [ -d googletest ]; then
			printf "  Google test exists, skipping."
		else
			git clone -b release-1.8.1 https://github.com/google/googletest.git
		fi
	fi
fi

cd .. #/..

# Set up dependencies
BUILD_DIR="MSVC${MSVC_DISPLAY_YEAR}_${BITS}"

if [ -n "$NMAKE" ]; then
	BUILD_DIR="${BUILD_DIR}_NMake_${BUILD_CONFIG}"
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


# Boost
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
		CWD_DRIVE_ROOT_BASH=$(echo "$CWD_DRIVE_ROOT" | sed "s,\\\\,/,g" | sed "s,\(.\):,/\\1,")
		if [ -d CWD_DRIVE_ROOT_BASH ]; then
			printf "Cannot continue, ${CWD_DRIVE_ROOT_BASH} aka ${CWD_DRIVE_ROOT} already exists. Please remove before re-running. ";
			exit 1;
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

		if [ $MSVC_REAL_VER -ge 15 ]; then
			LIB_SUFFIX="1"
		else
			LIB_SUFFIX="0"
		fi

		add_cmake_opts -DBOOST_ROOT="$BOOST_SDK" \
			-DBOOST_LIBRARYDIR="${BOOST_SDK}/lib${BITS}-msvc-${MSVC_VER}.${LIB_SUFFIX}"
		add_cmake_opts -DBoost_COMPILER="-${TOOLSET}"

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
printf "OpenAL-Soft 1.19.1... "
{
	if [ -d openal-soft-1.19.1-bin ]; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf openal-soft-1.19.1-bin
		eval 7z x -y OpenAL-Soft-1.19.1.zip $STRIP
	fi
	OPENAL_SDK="$(real_pwd)/openal-soft-1.19.1-bin"
	add_cmake_opts -DOPENAL_INCLUDE_DIR="${OPENAL_SDK}/include/AL" \
		-DOPENAL_LIBRARY="${OPENAL_SDK}/libs/Win${BITS}/OpenAL32.lib"
	add_runtime_dlls "$(pwd)/openal-soft-1.19.1-bin/bin/WIN${BITS}/soft_oal.dll:OpenAL32.dll"
	echo Done.
}
cd $DEPS
echo
# OSG
printf "OSG 3.4.2-experimental... "
{
	cd $DEPS_INSTALL
	if [ -d OSG ] && \
		grep "OPENSCENEGRAPH_MAJOR_VERSION    3" OSG/include/osg/Version > /dev/null && \
		grep "OPENSCENEGRAPH_MINOR_VERSION    4" OSG/include/osg/Version > /dev/null && \
		grep "OPENSCENEGRAPH_PATCH_VERSION    2" OSG/include/osg/Version > /dev/null
	then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf OSG
		eval 7z x -y "${DEPS}/OSG-3.4.2-experimental-msvc${MSVC_REAL_YEAR}-win${BITS}.7z" $STRIP
		[ -n "$PDBS" ] && eval 7z x -y "${DEPS}/OSG-3.4.2-experimental-msvc${MSVC_REAL_YEAR}-win${BITS}-sym.7z" $STRIP
		mv "OSG-3.4.2-experimental-msvc${MSVC_REAL_YEAR}-win${BITS}" OSG
	fi
	OSG_SDK="$(real_pwd)/OSG"
	add_cmake_opts -DOSG_DIR="$OSG_SDK"
	if [ $CONFIGURATION == "Debug" ]; then
		SUFFIX="d"
	else
		SUFFIX=""
	fi
	add_runtime_dlls "$(pwd)/OSG/bin/"{OpenThreads,zlib,libpng}${SUFFIX}.dll \
		"$(pwd)/OSG/bin/osg"{,Animation,DB,FX,GA,Particle,Text,Util,Viewer,Shadow}${SUFFIX}.dll
	add_osg_dlls "$(pwd)/OSG/bin/osgPlugins-3.4.2/osgdb_"{bmp,dds,freetype,jpeg,osg,png,tga}${SUFFIX}.dll
	add_osg_dlls "$(pwd)/OSG/bin/osgPlugins-3.4.2/osgdb_serializers_osg"{,animation,fx,ga,particle,text,util,viewer,shadow}${SUFFIX}.dll
	echo Done.
}
cd $DEPS
echo
# Qt
if [ -z $APPVEYOR ]; then
	printf "Qt 5.7.0... "
else
	printf "Qt 5.13 AppVeyor... "
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
		QT_SDK="C:/Qt/5.13/msvc2017${SUFFIX}"
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
printf "SDL 2.0.12... "
{
	if [ -d SDL2-2.0.12 ]; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf SDL2-2.0.12
		eval 7z x -y SDL2-2.0.12.zip $STRIP
	fi
	export SDL2DIR="$(real_pwd)/SDL2-2.0.12"
	add_runtime_dlls "$(pwd)/SDL2-2.0.12/lib/x${ARCHSUFFIX}/SDL2.dll"
	echo Done.
}
cd $DEPS
echo
# Google Test and Google Mock
if [ ! -z $TEST_FRAMEWORK ]; then
	printf "Google test 1.8.1 ..."

	cd googletest
	if [ ! -d build ]; then
		mkdir build
	fi

	cd build

	GOOGLE_INSTALL_ROOT="${DEPS_INSTALL}/GoogleTest"
	if [ $CONFIGURATION == "Debug" ]; then
			DEBUG_SUFFIX="d"
		else
			DEBUG_SUFFIX=""
	fi

	if [ ! -d $GOOGLE_INSTALL_ROOT ]; then

		cmake .. -DCMAKE_BUILD_TYPE="${CONFIGURATION}" -DCMAKE_INSTALL_PREFIX="${GOOGLE_INSTALL_ROOT}" -DCMAKE_USE_WIN32_THREADS_INIT=1 -G "${GENERATOR}" -DBUILD_SHARED_LIBS=1
		cmake --build . --config "${CONFIGURATION}"
		cmake --build . --target install --config "${CONFIGURATION}"

		add_runtime_dlls "${GOOGLE_INSTALL_ROOT}\bin\gtest_main${DEBUG_SUFFIX}.dll"
		add_runtime_dlls "${GOOGLE_INSTALL_ROOT}\bin\gtest${DEBUG_SUFFIX}.dll"
		add_runtime_dlls "${GOOGLE_INSTALL_ROOT}\bin\gmock_main${DEBUG_SUFFIX}.dll"
		add_runtime_dlls "${GOOGLE_INSTALL_ROOT}\bin\gmock${DEBUG_SUFFIX}.dll"
	fi

	add_cmake_opts -DBUILD_UNITTESTS=yes
	# FindGTest and FindGMock do not work perfectly on Windows
	# but we can help them by telling them everything we know about installation
	add_cmake_opts -DGMOCK_ROOT="$GOOGLE_INSTALL_ROOT"
	add_cmake_opts -DGTEST_ROOT="$GOOGLE_INSTALL_ROOT"
	add_cmake_opts -DGTEST_LIBRARY="$GOOGLE_INSTALL_ROOT/lib/gtest${DEBUG_SUFFIX}.lib"
	add_cmake_opts -DGTEST_MAIN_LIBRARY="$GOOGLE_INSTALL_ROOT/lib/gtest_main${DEBUG_SUFFIX}.lib"
	add_cmake_opts -DGMOCK_LIBRARY="$GOOGLE_INSTALL_ROOT/lib/gmock${DEBUG_SUFFIX}.lib"
	add_cmake_opts -DGMOCK_MAIN_LIBRARY="$GOOGLE_INSTALL_ROOT/lib/gmock_main${DEBUG_SUFFIX}.lib"
	add_cmake_opts -DGTEST_LINKED_AS_SHARED_LIBRARY=True
	echo Done.

fi

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
	DLL_PREFIX=""
	if [ -z $NMAKE ]; then
		mkdir -p $BUILD_CONFIG
		DLL_PREFIX="$BUILD_CONFIG/"
	fi
	for DLL in $RUNTIME_DLLS; do
		TARGET="$(basename "$DLL")"
		if [[ "$DLL" == *":"* ]]; then
			IFS=':'; SPLIT=( ${DLL} ); unset IFS
			DLL=${SPLIT[0]}
			TARGET=${SPLIT[1]}
		fi
		echo "    ${TARGET}."
		cp "$DLL" "${DLL_PREFIX}$TARGET"
	done
	echo
	echo "- OSG Plugin DLLs..."
	mkdir -p ${DLL_PREFIX}osgPlugins-3.4.2
	for DLL in $OSG_PLUGINS; do
		echo "    $(basename $DLL)."
		cp "$DLL" ${DLL_PREFIX}osgPlugins-3.4.2
	done
	echo
	echo "- Qt Platform DLLs..."
	mkdir -p ${DLL_PREFIX}platforms
	for DLL in $QT_PLATFORMS; do
		echo "    $(basename $DLL)"
		cp "$DLL" "${DLL_PREFIX}platforms"
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
