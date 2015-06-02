#!/bin/bash

while [ $# -gt 0 ]; do
	ARG=$1
	shift

	case $ARG in
		-v )
			VERBOSE=true ;;

		* )
			echo "Unknown arg $ARG."
			exit 1 ;;
	esac
done

if [ -z $VERBOSE ]; then
	STRIP="> /dev/null 2>&1"
fi

if [ -z $APPVEYOR ]; then
	echo "Running prebuild outside of Appveyor."

	DIR=$(echo "$0" | sed "s,\\\\,/,g" | sed "s,\(.\):,/\\1,")
	cd $(dirname "$DIR")
else
	echo "Running prebuild in Appveyor."

	cd $APPVEYOR_BUILD_FOLDER
	VERSION="$(cat README.md | grep Version: | awk '{ print $3; }')-$(git rev-parse --short HEAD)"
	appveyor UpdateBuild -Version "$VERSION"
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
				exit $RET
			else
				7z a output.7z output.log > /dev/null 2>&1

				appveyor PushArtifact output.7z -DeploymentName $CMD-output.7z
				appveyor AddMessage "Command $CMD failed (code $RET), output has been pushed as an artifact." -Category Error
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
	if ! [ -f $2 ]; then
		printf "  Downloading $2... "

		if [ -z $VERBOSE ]; then
			curl --silent --retry 10 -kLy 5 -o $2 $1
			RET=$?
		else
			curl --retry 10 -kLy 5 -o $2 $1
			RET=$?
		fi

		if [ $RET -ne 0 ]; then
			echo "Failed!"
		else
			echo "Done"
		fi

		return $RET
	else
		echo "  $2 exists, skipping."
	fi

	return 0
}

real_pwd() {
	pwd | sed "s,/\(.\),\1:,"
}

msbuild() {
	/c/Program\ Files\ \(x86\)/MSBuild/12.0/Bin/MSBuild.exe $@
}

CMAKE_OPTS=""
add_cmake_opts() {
	CMAKE_OPTS="$CMAKE_OPTS $@"
}

if [ -z "$ARCH" ]; then
	if [ -z "$PLATFORM" ]; then
		ARCH=`uname -m`
	else
		ARCH="$PLATFORM"
	fi
fi

case $PLATFORM in
	x64|x86_64|x86-64|win64|Win64 )
		ARCHNAME=x86-64
		ARCHSUFFIX=64
		BITS=64

		BASE_OPTS="-G\"Visual Studio 12 2013 Win64\""
		add_cmake_opts "-G\"Visual Studio 12 2013 Win64\""
		;;

	x32|x86|i686|i386|win32|Win32 )
		ARCHNAME=x86
		ARCHSUFFIX=86
		BITS=32

		BASE_OPTS="-G\"Visual Studio 12 2013\" -Tv120_xp"
		add_cmake_opts "-G\"Visual Studio 12 2013\"" -Tv120_xp
		;;

	* )
		echo "Unknown platform $PLATFORM."
		exit 1
		;;
esac

echo
echo "=========================="
echo "Starting prebuild on win$BITS"
echo "=========================="
echo

mkdir -p deps
cd deps

DEPS="`pwd`"

echo "Downloading dependency packages."
echo

# Boost
if [ -z $APPVEYOR ]; then
	echo "Boost 1.58.0..."
	download http://sourceforge.net/projects/boost/files/boost-binaries/1.58.0/boost_1_58_0-msvc-12.0-$BITS.exe boost-1.58.0-win$BITS.exe
	echo
fi

# Bullet
echo "Bullet 2.83.4..."
download http://www.lysator.liu.se/~ace/OpenMW/deps/Bullet-2.83.4-win$BITS.7z Bullet-2.83.4-win$BITS.7z
echo

# FFmpeg
echo "FFmpeg 2.5.2..."
download http://ffmpeg.zeranoe.com/builds/win$BITS/shared/ffmpeg-2.5.2-win$BITS-shared.7z ffmpeg$BITS-2.5.2.7z
download http://ffmpeg.zeranoe.com/builds/win$BITS/dev/ffmpeg-2.5.2-win$BITS-dev.7z ffmpeg$BITS-2.5.2-dev.7z
echo

# MyGUI
echo "MyGUI 3.2.2..."
download http://www.lysator.liu.se/~ace/OpenMW/deps/MyGUI-3.2.2-win$BITS.7z MyGUI-3.2.2-win$BITS.7z
echo

# Ogre
echo "Ogre 1.9..."
download http://www.lysator.liu.se/~ace/OpenMW/deps/Ogre-1.9-win$BITS.7z Ogre-1.9-win$BITS.7z
echo

# OpenAL
echo "OpenAL-Soft 1.16.0..."
download http://kcat.strangesoft.net/openal-soft-1.16.0-bin.zip OpenAL-Soft-1.16.0.zip
echo

# Qt
echo "Qt 4.8.6..."
download http://sourceforge.net/projects/qt64ng/files/qt/$ARCHNAME/4.8.6/msvc2013/qt-4.8.6-x$ARCHSUFFIX-msvc2013.7z qt$BITS-4.8.6.7z
echo

# SDL2
echo "SDL 2.0.3 binaries..."
download https://www.libsdl.org/release/SDL2-devel-2.0.3-VC.zip SDL2-2.0.3.zip
echo

cd ..

# Set up dependencies
rm -rf build_$BITS
mkdir -p build_$BITS/deps
cd deps

echo
echo "Extracting dependencies..."

# Boost
if [ -z $APPVEYOR ]; then
	printf "Boost 1.58.0... "
	cd ../build_$BITS/deps

	BOOST_SDK="`real_pwd`/Boost"

	$DEPS/boost-1.58.0-win$BITS.exe //dir="$BOOST_SDK" //verysilent

	add_cmake_opts -DBOOST_ROOT="$BOOST_SDK" \
		-DBOOST_LIBRARYDIR="$BOOST_SDK/lib$BITS-msvc-12.0"

	cd $DEPS

	echo Done.
else
	# Appveyor unstable has all the boost we need already
	BOOST_SDK="c:/Libraries/boost"
	add_cmake_opts -DBOOST_ROOT="$BOOST_SDK" \
		-DBOOST_LIBRARYDIR="$BOOST_SDK/lib$BITS-msvc-12.0"
fi

# Bullet
printf "Bullet 2.83.4... "
cd ../build_$BITS/deps

eval 7z x -y $DEPS/Bullet-2.83.4-win$BITS.7z $STRIP
mv Bullet-2.83.4-win$BITS Bullet

BULLET_SDK="`real_pwd`/Bullet"
add_cmake_opts -DBULLET_INCLUDE_DIR="$BULLET_SDK/include" \
	-DBULLET_COLLISION_LIBRARY="$BULLET_SDK/lib/BulletCollision.lib" \
	-DBULLET_COLLISION_LIBRARY_DEBUG="$BULLET_SDK/lib/BulletCollision_Debug.lib" \
	-DBULLET_DYNAMICS_LIBRARY="$BULLET_SDK/lib/BulletDynamics.lib" \
	-DBULLET_DYNAMICS_LIBRARY_DEBUG="$BULLET_SDK/lib/BulletDynamics_Debug.lib" \
	-DBULLET_MATH_LIBRARY="$BULLET_SDK/lib/LinearMath.lib" \
	-DBULLET_MATH_LIBRARY_DEBUG="$BULLET_SDK/lib/LinearMath_Debug.lib"

cd $DEPS

echo Done.

# FFmpeg
printf "FFmpeg 2.5.2... "
cd ../build_$BITS/deps

eval 7z x -y $DEPS/ffmpeg$BITS-2.5.2.7z $STRIP
eval 7z x -y $DEPS/ffmpeg$BITS-2.5.2-dev.7z $STRIP

mv ffmpeg-2.5.2-win$BITS-shared FFmpeg
cp -r ffmpeg-2.5.2-win$BITS-dev/* FFmpeg/
rm -rf ffmpeg-2.5.2-win$BITS-dev

FFMPEG_SDK="`real_pwd`/FFmpeg"
add_cmake_opts -DAVCODEC_INCLUDE_DIRS="$FFMPEG_SDK/include" \
	-DAVCODEC_LIBRARIES="$FFMPEG_SDK/lib/avcodec.lib" \
	-DAVDEVICE_INCLUDE_DIRS="$FFMPEG_SDK/include" \
	-DAVDEVICE_LIBRARIES="$FFMPEG_SDK/lib/avdevice.lib" \
	-DAVFORMAT_INCLUDE_DIRS="$FFMPEG_SDK/include" \
	-DAVFORMAT_LIBRARIES="$FFMPEG_SDK/lib/avformat.lib" \
	-DAVUTIL_INCLUDE_DIRS="$FFMPEG_SDK/include" \
	-DAVUTIL_LIBRARIES="$FFMPEG_SDK/lib/avutil.lib" \
	-DPOSTPROC_INCLUDE_DIRS="$FFMPEG_SDK/include" \
	-DPOSTPROC_LIBRARIES="$FFMPEG_SDK/lib/postproc.lib" \
	-DSWRESAMPLE_INCLUDE_DIRS="$FFMPEG_SDK/include" \
	-DSWRESAMPLE_LIBRARIES="$FFMPEG_SDK/lib/swresample.lib" \
	-DSWSCALE_INCLUDE_DIRS="$FFMPEG_SDK/include" \
	-DSWSCALE_LIBRARIES="$FFMPEG_SDK/lib/swscale.lib"

if [ $BITS -eq 32 ]; then
	add_cmake_opts "-DCMAKE_EXE_LINKER_FLAGS=\"/machine:X86 /safeseh:no\""
fi

cd $DEPS

echo Done.

# Ogre
printf "Ogre 1.9... "
cd ../build_$BITS/deps

eval 7z x -y $DEPS/Ogre-1.9-win$BITS.7z $STRIP
mv Ogre-1.9-win$BITS Ogre

OGRE_SDK="`real_pwd`/Ogre"

add_cmake_opts -DOGRE_SDK="$OGRE_SDK"

cd $DEPS

echo Done.

# MyGUI
printf "MyGUI 3.2.2... "
cd ../build_$BITS/deps

eval 7z x -y $DEPS/MyGUI-3.2.2-win$BITS.7z $STRIP
mv MyGUI-3.2.2-win$BITS MyGUI

MYGUI_SDK="`real_pwd`/MyGUI"

add_cmake_opts -DMYGUISDK="$MYGUI_SDK" \
	-DMYGUI_PLATFORM_INCLUDE_DIRS="$MYGUI_SDK/include/MYGUI" \
	-DMYGUI_INCLUDE_DIRS="$MYGUI_SDK/include" \
	-DMYGUI_PREQUEST_FILE="$MYGUI_SDK/include/MYGUI/MyGUI_Prerequest.h"

cd $DEPS

echo Done.

# OpenAL
printf "OpenAL-Soft 1.16.0... "
eval 7z x -y OpenAL-Soft-1.16.0.zip $STRIP

OPENAL_SDK="`real_pwd`/openal-soft-1.16.0-bin"

add_cmake_opts -DOPENAL_INCLUDE_DIR="$OPENAL_SDK/include" \
	-DOPENAL_LIBRARY="$OPENAL_SDK/libs/Win$BITS/OpenAL32.lib"

echo Done.

# Qt
printf "Qt 4.8.6... "
cd ../build_$BITS/deps

eval 7z x -y $DEPS/qt$BITS-4.8.6.7z $STRIP
mv qt-4.8.6-* Qt

QT_SDK="`real_pwd`/Qt"

cd $QT_SDK
eval qtbinpatcher.exe $STRIP

add_cmake_opts -DQT_QMAKE_EXECUTABLE="$QT_SDK/bin/qmake.exe"

cd $DEPS

echo Done.

# SDL2
printf "SDL 2.0.3... "
eval 7z x -y SDL2-2.0.3.zip $STRIP

SDL_SDK="`real_pwd`/SDL2-2.0.3"
add_cmake_opts  -DSDL2_INCLUDE_DIR="$SDL_SDK/include" \
	-DSDL2MAIN_LIBRARY="$SDL_SDK/lib/x$ARCHSUFFIX/SDL2main.lib" \
	-DSDL2_LIBRARY_PATH="$SDL_SDK/lib/x$ARCHSUFFIX/SDL2.lib" \
	-DSDL2_LIBRARY_ONLY="$SDL_SDK/lib/x$ARCHSUFFIX/SDL2.lib"

cd $DEPS

echo Done.
echo

cd ../build_$BITS

echo "Building OpenMW..."

add_cmake_opts -DBUILD_BSATOOL=no \
	-DBUILD_ESMTOOL=no \
	-DBUILD_MYGUI_PLUGIN=no

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

exit $RET