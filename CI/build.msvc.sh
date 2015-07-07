#!/bin/bash

if [ -z $PLATFORM ]; then
	PLATFORM=`uname -m`
fi

if [ -z $CONFIGURATION ]; then
	CONFIGURATION="Debug"
fi

case $PLATFORM in
	x32|x86|i686|i386|win32|Win32 )
		BITS=32
		PLATFORM=Win32
		;;

	x64|x86_64|x86-64|win64|Win64 )
		BITS=64
		PLATFORM=x64
		;;

	* )
		echo "Unknown platform $PLATFORM."
		exit 1 ;;
esac

if [ -z $APPVEYOR ]; then
	echo "Running $BITS-bit $CONFIGURATION build outside of Appveyor."

	DIR=$(echo "$0" | sed "s,\\\\,/,g" | sed "s,\(.\):,/\\1,")
	cd $(dirname "$DIR")/..
else
	echo "Running $BITS-bit $CONFIGURATION build in Appveyor."

	cd $APPVEYOR_BUILD_FOLDER
fi

cd build_$BITS

which msbuild > /dev/null
if [ $? -ne 0 ]; then
	msbuild() {
		/c/Program\ Files\ \(x86\)/MSBuild/12.0/Bin/MSBuild.exe "$@"
	}
fi

if [ -z $APPVEYOR ]; then
	msbuild OpenMW.sln //t:Build //m:8
else
	msbuild OpenMW.sln //t:Build //m:8 //logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"
fi

RET=$?
if [ $RET -eq 0 ] && [ ! -z $PACKAGE ]; then
	msbuild PACKAGE.vcxproj //t:Build //m:8
	RET=$?
fi

exit $RET
