#!/bin/bash

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

			v )
				VS_VERSION=$1
				shift ;;

			d )
				SKIP_DOWNLOAD=true ;;

			e )
				SKIP_EXTRACT=true ;;

			k )
				KEEP=true ;;

			u )
				UNITY_BUILD=true ;;

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
	-v <2013/2015>
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
if [ -z $VS_VERSION ]; then
	VS_VERSION="2013"
fi

if [ -z $APPVEYOR ]; then
	echo "Running prebuild outside of Appveyor."

	DIR=$(echo "$0" | sed "s,\\\\,/,g" | sed "s,\(.\):,/\\1,")
	cd $(dirname "$DIR")/..
else
	echo "Running prebuild in Appveyor."

	cd $APPVEYOR_BUILD_FOLDER
	VERSION="$(cat README.md | grep Version: | awk '{ print $3; }')-$(git rev-parse --short HEAD)"
	appveyor UpdateBuild -Version "$VERSION" > /dev/null &
fi

run_cmd() {
	CMD="$1"
	shift

	if [ -z $VERBOSE ]; then
		eval $CMD $@ > output.log 2>&1
		RET=$?

		if [ $RET -ne 0 ]; then
			if [ -z $APPVEYOR ]; then
				echo "Command $CMD failed, output can be found in `real_pwd`/output.log"
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

if [ -z $PLATFORM ]; then
	PLATFORM=`uname -m`
fi

if [ -z $CONFIGURATION ]; then
	CONFIGURATION="Debug"
fi

case $VS_VERSION in
	14|2015 )
		GENERATOR="Visual Studio 14 2015"
		XP_TOOLSET="v140_xp"
		;;

#	12|2013|
	* )
		GENERATOR="Visual Studio 12 2013"
		XP_TOOLSET="v120_xp"
		;;
esac

case $PLATFORM in
	x64|x86_64|x86-64|win64|Win64 )
		ARCHNAME=x86-64
		ARCHSUFFIX=64
		BITS=64

		BASE_OPTS="-G\"$GENERATOR Win64\""
		add_cmake_opts "-G\"$GENERATOR Win64\""
		;;

	x32|x86|i686|i386|win32|Win32 )
		ARCHNAME=x86
		ARCHSUFFIX=86
		BITS=32

		BASE_OPTS="-G\"$GENERATOR\" -T$XP_TOOLSET"
		add_cmake_opts "-G\"$GENERATOR\"" -T$XP_TOOLSET
		;;

	* )
		echo "Unknown platform $PLATFORM."
		exit 1
		;;
esac

if ! [ -z $UNITY_BUILD ]; then
	add_cmake_opts "-DOPENMW_UNITY_BUILD=True"
fi

case $CONFIGURATION in
	debug|Debug|DEBUG )
		CONFIGURATION=Debug
		;;

	release|Release|RELEASE )
		CONFIGURATION=Release
		;;

	relwithdebinfo|RelWithDebInfo|RELWITHDEBINFO )
		CONFIGURATION=RelWithDebInfo
		;;
esac

echo
echo "=========================="
echo "Starting prebuild on win$BITS"
echo "=========================="
echo

# cd OpenMW/AppVeyor-test
mkdir -p deps
cd deps

DEPS="`pwd`"

if [ -z $SKIP_DOWNLOAD ]; then
	echo "Downloading dependency packages."
	echo

	# Boost
	if [ -z $APPVEYOR ]; then
		download "Boost 1.58.0" \
			http://sourceforge.net/projects/boost/files/boost-binaries/1.58.0/boost_1_58_0-msvc-12.0-$BITS.exe \
			boost-1.58.0-win$BITS.exe
	fi

	# Bullet
	download "Bullet 2.83.5" \
		http://www.lysator.liu.se/~ace/OpenMW/deps/Bullet-2.83.5-win$BITS.7z \
		Bullet-2.83.5-win$BITS.7z

	# FFmpeg
	download "FFmpeg 2.5.2" \
		http://ffmpeg.zeranoe.com/builds/win$BITS/shared/ffmpeg-2.5.2-win$BITS-shared.7z \
		ffmpeg$BITS-2.5.2.7z \
		http://ffmpeg.zeranoe.com/builds/win$BITS/dev/ffmpeg-2.5.2-win$BITS-dev.7z \
		ffmpeg$BITS-2.5.2-dev.7z

	# MyGUI
	download "MyGUI 3.2.2" \
		http://www.lysator.liu.se/~ace/OpenMW/deps/MyGUI-3.2.2-win$BITS.7z \
		MyGUI-3.2.2-win$BITS.7z

	# OpenAL
	download "OpenAL-Soft 1.16.0" \
		http://kcat.strangesoft.net/openal-binaries/openal-soft-1.16.0-bin.zip \
		OpenAL-Soft-1.16.0.zip

	# OSG
	download "OpenSceneGraph 3.3.8" \
		http://www.lysator.liu.se/~ace/OpenMW/deps/OSG-3.3.8-win$BITS.7z \
		OSG-3.3.8-win$BITS.7z

	# Qt
	if [ -z $APPVEYOR ]; then
		download "Qt 4.8.6" \
			http://sourceforge.net/projects/qt64ng/files/qt/$ARCHNAME/4.8.6/msvc2013/qt-4.8.6-x$ARCHSUFFIX-msvc2013.7z \
			qt$BITS-4.8.6.7z
	fi

	# SDL2
	download "SDL 2.0.3" \
		https://www.libsdl.org/release/SDL2-devel-2.0.3-VC.zip \
		SDL2-2.0.3.zip
fi

cd .. #/..

# Set up dependencies
if [ -z $KEEP ]; then
	echo
	printf "Preparing build directory... "

	rm -rf Build_$BITS
	mkdir -p Build_$BITS/deps

	echo Done.
fi
mkdir -p Build_$BITS/deps
cd Build_$BITS/deps

DEPS_INSTALL=`pwd`
cd $DEPS

echo
echo "Extracting dependencies..."


# Boost
printf "Boost 1.58.0... "
{
	if [ -z $APPVEYOR ]; then
		cd $DEPS_INSTALL

		BOOST_SDK="`real_pwd`/Boost"

		if [ -d Boost ] && grep "BOOST_VERSION 105800" Boost/boost/version.hpp > /dev/null; then
			printf "Exists. "
		elif [ -z $SKIP_EXTRACT ]; then
			rm -rf Boost
			$DEPS/boost-1.58.0-win$BITS.exe //dir="$(echo $BOOST_SDK | sed s,/,\\\\,g)" //verysilent
		fi

		add_cmake_opts -DBOOST_ROOT="$BOOST_SDK" \
			-DBOOST_LIBRARYDIR="$BOOST_SDK/lib$BITS-msvc-12.0"

		echo Done.
	else
		# Appveyor unstable has all the boost we need already
		BOOST_SDK="c:/Libraries/boost"
		add_cmake_opts -DBOOST_ROOT="$BOOST_SDK" \
			-DBOOST_LIBRARYDIR="$BOOST_SDK/lib$BITS-msvc-12.0"

		echo AppVeyor.
	fi
}
cd $DEPS

# Bullet
printf "Bullet 2.83.5... "
{
	cd $DEPS_INSTALL

	if [ -d Bullet ]; then
		printf "Exists. (No version checking) "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf Bullet
		eval 7z x -y $DEPS/Bullet-2.83.5-win$BITS.7z $STRIP
		mv Bullet-2.83.5-win$BITS Bullet
	fi

	export BULLET_ROOT="`real_pwd`/Bullet"

	echo Done.
}
cd $DEPS

# FFmpeg
printf "FFmpeg 2.5.2... "
{
	cd $DEPS_INSTALL

	if [ -d FFmpeg ] && grep "FFmpeg version: 2.5.2" FFmpeg/README.txt > /dev/null; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf FFmpeg

		eval 7z x -y $DEPS/ffmpeg$BITS-2.5.2.7z $STRIP
		eval 7z x -y $DEPS/ffmpeg$BITS-2.5.2-dev.7z $STRIP

		mv ffmpeg-2.5.2-win$BITS-shared FFmpeg
		cp -r ffmpeg-2.5.2-win$BITS-dev/* FFmpeg/
		rm -rf ffmpeg-2.5.2-win$BITS-dev
	fi

	export FFMPEG_HOME="`real_pwd`/FFmpeg"
	add_runtime_dlls `pwd`/FFmpeg/bin/{avcodec-56,avformat-56,avutil-54,swresample-1,swscale-3}.dll

	if [ $BITS -eq 32 ]; then
		add_cmake_opts "-DCMAKE_EXE_LINKER_FLAGS=\"/machine:X86 /safeseh:no\""
	fi

	echo Done.
}
cd $DEPS

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
		eval 7z x -y $DEPS/MyGUI-3.2.2-win$BITS.7z $STRIP
		mv MyGUI-3.2.2-win$BITS MyGUI
	fi

	MYGUI_SDK="`real_pwd`/MyGUI"

	add_cmake_opts -DMYGUISDK="$MYGUI_SDK" \
		-DMYGUI_INCLUDE_DIRS="$MYGUI_SDK/include/MYGUI" \
		-DMYGUI_PREQUEST_FILE="$MYGUI_SDK/include/MYGUI/MyGUI_Prerequest.h"

	if [ $CONFIGURATION == "Debug" ]; then
		SUFFIX="_d"
	else
		SUFFIX=""
	fi
	add_runtime_dlls `pwd`/MyGUI/bin/$CONFIGURATION/MyGUIEngine$SUFFIX.dll

	echo Done.
}
cd $DEPS

# OpenAL
printf "OpenAL-Soft 1.16.0... "
{
	if [ -d openal-soft-1.16.0-bin ]; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf openal-soft-1.16.0-bin
		eval 7z x -y OpenAL-Soft-1.16.0.zip $STRIP
	fi

	OPENAL_SDK="`real_pwd`/openal-soft-1.16.0-bin"

	add_cmake_opts -DOPENAL_INCLUDE_DIR="$OPENAL_SDK/include/AL" \
		-DOPENAL_LIBRARY="$OPENAL_SDK/libs/Win$BITS/OpenAL32.lib"

	echo Done.
}
cd $DEPS

# OSG
printf "OSG 3.3.8... "
{
	cd $DEPS_INSTALL

	if [ -d OSG ] && \
		grep "OPENSCENEGRAPH_MAJOR_VERSION    3" OSG/include/osg/Version > /dev/null && \
		grep "OPENSCENEGRAPH_MINOR_VERSION    3" OSG/include/osg/Version > /dev/null && \
		grep "OPENSCENEGRAPH_PATCH_VERSION    8" OSG/include/osg/Version > /dev/null
	then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf OSG
		eval 7z x -y $DEPS/OSG-3.3.8-win$BITS.7z $STRIP
		mv OSG-3.3.8-win$BITS OSG
	fi

	OSG_SDK="`real_pwd`/OSG"

	add_cmake_opts -DOSG_DIR="$OSG_SDK"

	if [ $CONFIGURATION == "Debug" ]; then
		SUFFIX="d"
	else
		SUFFIX=""
	fi

	add_runtime_dlls `pwd`/OSG/bin/{OpenThreads,zlib}$SUFFIX.dll \
		`pwd`/OSG/bin/osg{,Animation,DB,FX,GA,Particle,Qt,Text,Util,Viewer}$SUFFIX.dll

	add_osg_dlls `pwd`/OSG/bin/osgPlugins-3.3.8/osgdb_{bmp,dds,gif,jpeg,png,tga}$SUFFIX.dll

	echo Done.
}
cd $DEPS

# Qt
if [ -z $APPVEYOR ]; then
	printf "Qt 4.8.6... "
else
	printf "Qt 5.4... "
fi
{
	if [ -z $APPVEYOR ]; then
		cd $DEPS_INSTALL
		QT_SDK="`real_pwd`/Qt"

		if [ -d Qt ] && head -n2 Qt/BUILDINFO.txt | grep "4.8.6" > /dev/null; then
			printf "Exists. "
		elif [ -z $SKIP_EXTRACT ]; then
			rm -rf Qt
			eval 7z x -y $DEPS/qt$BITS-4.8.6.7z $STRIP
			mv qt-4.8.6-* Qt
			cd Qt
			eval ./qtbinpatcher.exe $STRIP
		fi

		cd $QT_SDK

		add_cmake_opts -DDESIRED_QT_VERSION=4 \
			-DQT_QMAKE_EXECUTABLE="$QT_SDK/bin/qmake.exe"

		if [ $CONFIGURATION == "Debug" ]; then
			SUFFIX="d4"
		else
			SUFFIX="4"
		fi

		add_runtime_dlls `pwd`/bin/Qt{Core,Gui,Network,OpenGL}$SUFFIX.dll

		echo Done.
	else
		if [ $BITS -eq 32 ]; then
			QT_SDK="C:/Qt/5.4/msvc2013_opengl"
		else
			QT_SDK="C:/Qt/5.4/msvc2013_64_opengl"
		fi

		add_cmake_opts -DDESIRED_QT_VERSION=5 \
			-DQT_QMAKE_EXECUTABLE="$QT_SDK/bin/qmake.exe" \
			-DCMAKE_PREFIX_PATH="$QT_SDK"

		echo AppVeyor.
	fi
}
cd $DEPS

# SDL2
printf "SDL 2.0.3... "
{
	if [ -d SDL2-2.0.3 ]; then
		printf "Exists. "
	elif [ -z $SKIP_EXTRACT ]; then
		rm -rf SDL2-2.0.3
		eval 7z x -y SDL2-2.0.3.zip $STRIP
	fi

	SDL_SDK="`real_pwd`/SDL2-2.0.3"
	add_cmake_opts -DSDL2_INCLUDE_DIR="$SDL_SDK/include" \
		-DSDL2MAIN_LIBRARY="$SDL_SDK/lib/x$ARCHSUFFIX/SDL2main.lib" \
		-DSDL2_LIBRARY_PATH="$SDL_SDK/lib/x$ARCHSUFFIX/SDL2.lib"

	add_runtime_dlls `pwd`/SDL2-2.0.3/lib/x$ARCHSUFFIX/SDL2.dll

	echo Done.
}


cd $DEPS_INSTALL/..

echo
echo "Setting up OpenMW build..."

add_cmake_opts -DBUILD_BSATOOL=no \
	-DBUILD_ESMTOOL=no \
	-DBUILD_MYGUI_PLUGIN=no \
	-DOPENMW_MP_BUILD=on

if [ -z $CI ]; then
	echo "  (Outside of CI, doing full build.)"
else
	case $STEP in
		components )
			echo "  Subproject: Components."
			add_cmake_opts -DBUILD_ESSIMPORTER=no \
				-DBUILD_LAUNCHER=no \
				-DBUILD_MWINIIMPORTER=no \
				-DBUILD_OPENCS=no \
				-DBUILD_OPENMW=no \
				-DBUILD_WIZARD=no
			;;
		openmw )
			echo "  Subproject: OpenMW."
			add_cmake_opts -DBUILD_ESSIMPORTER=no \
				-DBUILD_LAUNCHER=no \
				-DBUILD_MWINIIMPORTER=no \
				-DBUILD_OPENCS=no \
				-DBUILD_WIZARD=no
			;;
		opencs )
			echo "  Subproject: OpenCS."
			add_cmake_opts -DBUILD_ESSIMPORTER=no \
				-DBUILD_LAUNCHER=no \
				-DBUILD_MWINIIMPORTER=no \
				-DBUILD_OPENMW=no \
				-DBUILD_WIZARD=no
			;;
		misc )
			echo "  Subproject: Misc."
			add_cmake_opts -DBUILD_OPENCS=no \
				-DBUILD_OPENMW=no
			;;
		* )
			echo "  Building everything."
			;;
	esac
fi

if [ -z $VERBOSE ]; then
	printf "  Configuring... "
else
	echo "  cmake .. $CMAKE_OPTS"
fi

run_cmd cmake .. $CMAKE_OPTS
RET=$?

if [ -z $VERBOSE ]; then
	if [ $RET -eq 0 ]; then echo Done.
	else echo Failed.; fi
fi

echo

# NOTE: Disable this when/if we want to run test cases
if [ -z $CI ]; then
	echo "Copying Runtime DLLs..."
	mkdir -p $CONFIGURATION
	for DLL in $RUNTIME_DLLS; do
		echo "  `basename $DLL`."
		cp "$DLL" $CONFIGURATION/
	done
	echo "OSG Plugin DLLs..."
	mkdir -p $CONFIGURATION/osgPlugins-3.3.8
	for DLL in $OSG_PLUGINS; do
		echo "  `basename $DLL`."
		cp "$DLL" $CONFIGURATION/osgPlugins-3.3.8
	done
	echo
	
	echo "Copying Runtime Resources/Config Files"
	
	echo "  gamecontrollerdb.txt"
	cp $CONFIGURATION/../gamecontrollerdb.txt $CONFIGURATION/gamecontrollerdb.txt
	echo "  openmw.cfg"
	cp $CONFIGURATION/../openmw.cfg.install $CONFIGURATION/openmw.cfg
	echo "  openmw-cs.cfg"
	cp $CONFIGURATION/../openmw-cs.cfg $CONFIGURATION/openmw-cs.cfg
	echo "  settings-default.cfg"
	cp $CONFIGURATION/../settings-default.cfg $CONFIGURATION/settings-default.cfg
	echo "  resources/"
	cp -r $CONFIGURATION/../resources $CONFIGURATION/resources
fi

exit $RET
