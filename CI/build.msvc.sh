#!/bin/bash

APPVEYOR=""
CI=""

PACKAGE=""
PLATFORM=""
CONFIGURATION=""
VS_VERSION=""

if [ -z $PLATFORM ]; then
	PLATFORM=`uname -m`
fi

if [ -z $CONFIGURATION ]; then
	CONFIGURATION="Debug"
fi

case $VS_VERSION in
	14|14.0|2015 )
		GENERATOR="Visual Studio 14 2015"
		MSVC_YEAR="2015"
		MSVC_VER="14.0"
		;;

#	12|2013|
	* )
		GENERATOR="Visual Studio 12 2013"
		MSVC_YEAR="2013"
		MVSC_VER="12.0"
		;;
esac

case $PLATFORM in
	x64|x86_64|x86-64|win64|Win64 )
		BITS=64
		;;

	x32|x86|i686|i386|win32|Win32 )
		BITS=32
		;;
esac

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

if [ -z $APPVEYOR ]; then
	echo "Running ${BITS}-bit MSVC${MSVC_YEAR} ${CONFIGURATION} build outside of Appveyor."

	DIR=$(echo "$0" | sed "s,\\\\,/,g" | sed "s,\(.\):,/\\1,")
	cd $(dirname "$DIR")/..
else
	echo "Running ${BITS}-bit MSVC${MSVC_YEAR} ${CONFIGURATION} build in Appveyor."

	cd $APPVEYOR_BUILD_FOLDER
fi

BUILD_DIR="MSVC${MSVC_YEAR}_${BITS}"
cd ${BUILD_DIR}

which msbuild > /dev/null
if [ $? -ne 0 ]; then
	msbuild() {
		/c/Program\ Files\ \(x86\)/MSBuild/${MSVC_VER}/Bin/MSBuild.exe "$@"
	}
fi

if [ -z $APPVEYOR ]; then
	msbuild OpenMW.sln //t:Build //p:Configuration=${CONFIGURATION} //m:8
else
	msbuild OpenMW.sln //t:Build //p:Configuration=${CONFIGURATION} //m:8 //logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"
fi

RET=$?
if [ $RET -eq 0 ] && [ ! -z $PACKAGE ]; then
	msbuild PACKAGE.vcxproj //t:Build //m:8
	RET=$?
fi

exit $RET
