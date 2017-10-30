#!/bin/sh
# explicitly use gcc7
sudo ln -s /usr/bin/gcc-7 /usr/local/bin/gcc
sudo ln -s /usr/bin/g++-7 /usr/local/bin/g++

# explicitly use clang5
sudo ln -s /usr/bin/clang-5.0 /usr/local/bin/clang
sudo ln -s /usr/bin/clang++-5.0 /usr/local/bin/clang++

# build libgtest & libgtest_main
sudo mkdir /usr/src/gtest/build
cd /usr/src/gtest/build
sudo cmake .. -DBUILD_SHARED_LIBS=1
sudo make -j4
sudo ln -s /usr/src/gtest/build/libgtest.so /usr/lib/libgtest.so
sudo ln -s /usr/src/gtest/build/libgtest_main.so /usr/lib/libgtest_main.so
