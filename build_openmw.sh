#!/bin/sh

# See INSTALL-linux.txt for instructions

make || exit 1

gdc -Wall -Wextra -O2 -g -fversion=Posix -o openmw openmw.d core/*.d ogre/*.d nif/*.d util/*.d bsa/*.d monster/util/*.d input/*.d sound/*.d scene/*.d esm/*.d cpp_*.o -laudiere -lm -lOgreMain -lOIS -lstdc++

gdc -Wall -Wextra -O2 -g -fversion=Posix -o esmtool esmtool.d core/*.d ogre/*.d nif/*.d util/*.d bsa/*.d monster/util/*.d input/*.d sound/*.d scene/*.d esm/*.d cpp_*.o -laudiere -lm -lOgreMain -lOIS -lstdc++
