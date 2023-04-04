#!/bin/bash -ex

git ls-files -- ':(exclude)extern/' 'CMakeLists.txt' '*.cmake' |
    xargs grep -P '^\s*\t' &&
    ( echo 'CMake files contain leading tab character. Use only spaces for indentation'; exit -1 )
exit 0
