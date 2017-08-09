#!/bin/sh
sudo ln -s /usr/bin/clang-3.6 /usr/local/bin/clang
sudo ln -s /usr/bin/clang++-3.6 /usr/local/bin/clang++

# build libgtest & libgtest_main
sudo mkdir /usr/src/gtest/build
cd /usr/src/gtest/build
sudo cmake .. -DBUILD_SHARED_LIBS=1
sudo make -j4
sudo ln -s /usr/src/gtest/build/libgtest.so /usr/lib/libgtest.so
sudo ln -s /usr/src/gtest/build/libgtest_main.so /usr/lib/libgtest_main.so
