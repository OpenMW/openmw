#!/bin/sh

free -m
mkdir build
cd build
export CODE_COVERAGE=1
if [ "${CC}" = "clang" ]; then export CODE_COVERAGE=0; fi
${ANALYZE}cmake .. -DBUILD_WITH_CODE_COVERAGE=${CODE_COVERAGE} -DBUILD_UNITTESTS=1 -DCMAKE_INSTALL_PREFIX=/usr -DBINDIR=/usr/games  -DCMAKE_BUILD_TYPE="None" -DUSE_SYSTEM_TINYXML=TRUE
