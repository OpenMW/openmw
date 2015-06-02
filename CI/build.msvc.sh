#!/bin/bash

case $PLATFORM in
	x32|x86|i686|i386|win32|Win32 )
		BITS=32 ;;

	x64|x86_64|x86-64|win64|Win64 )
		BITS=64 ;;

	* )
		echo "Unknown platform $PLATFORM."
		exit 1 ;;
esac

cd $(dirname $0)/../build_$BITS

msbuild OpenMW.sln //t:Build //p:Configuration=$CONFIGURATION //m:8 //logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"
