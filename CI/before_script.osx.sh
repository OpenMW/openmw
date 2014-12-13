#!/bin/sh

mkdir build
cd build
cmake -DCMAKE_FRAMEWORK_PATH="/usr/local/lib/macosx/Release" -DCMAKE_EXE_LINKER_FLAGS="-F/usr/local/lib/macosx/Release" -DCMAKE_CXX_FLAGS="-stdlib=libstdc++" -DCMAKE_BUILD_TYPE=Debug -DBUILD_MYGUI_PLUGIN=OFF -G"Unix Makefiles" ..
