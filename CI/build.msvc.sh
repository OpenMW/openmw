#!/bin/bash

case $1 in
	x86|i686|win32 )
		BITS=32 ;;

	x64_64|x64|win64 )
		BITS=64 ;;

	* )
		echo "Unknown platform $ARG."
		exit 1 ;;
esac

cd $(dirname $0)/../build_$BITS

msbuild OpenMW.sln //t:Build //p:Configuration=$2 //m:8 //logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"
