#!/bin/sh

# See COMPILE-linux.txt for instructions

make || exit 1

gdc -Wall -g -fversion=Posix -o openmw openmw.d core/*.d ogre/*.d nif/*.d util/*.d bsa/*.d monster/util/*.d input/*.d sound/*.d scene/*.d esm/*.d cpp_ogre.o -lalut -lopenal -lm -lOgreMain -lOIS -lstdc++

gdc -Wall -g -fversion=Posix -o esmtool esmtool.d core/*.d ogre/*.d nif/*.d util/*.d bsa/*.d monster/util/*.d input/*.d sound/*.d scene/*.d esm/*.d cpp_ogre.o -lalut -lopenal -lm -lOgreMain -lOIS -lstdc++
