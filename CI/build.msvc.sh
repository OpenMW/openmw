#!/bin/bash

while [ $# -gt 0 ]; do
	ARG=$1
	shift

	case $ARG in
		x86|i686|win32 )
			BITS=32 ;;

		x64_64|x64|win64 )
			BITS=64 ;;

		* )
			echo "Unknown arg $ARG."
			exit 1 ;;
	esac
done

cd $(dirname $0)/../build_$BITS

msbuild OpenMW.sln //t:Build //p:Configuration=Release //m:8 //logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"
