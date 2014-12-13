#!/bin/sh

export CXX=g++
export CC=gcc

echo "yes" | sudo add-apt-repository "deb http://archive.ubuntu.com/ubuntu `lsb_release -sc` main universe restricted multiverse"
echo "yes" | sudo apt-add-repository ppa:openmw/openmw
sudo apt-get update -qq
sudo apt-get install -qq libgtest-dev google-mock
sudo apt-get install -qq libboost-filesystem-dev libboost-program-options-dev libboost-system-dev libboost-thread-dev libboost-wave-dev
sudo apt-get install -qq libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libavresample-dev
sudo apt-get install -qq libbullet-dev libogre-1.9-dev libmygui-dev libsdl2-dev libunshield-dev libtinyxml-dev libopenal-dev libqt4-dev
sudo mkdir /usr/src/gtest/build
cd /usr/src/gtest/build
sudo cmake .. -DBUILD_SHARED_LIBS=1
sudo make -j4
sudo ln -s /usr/src/gtest/build/libgtest.so /usr/lib/libgtest.so
sudo ln -s /usr/src/gtest/build/libgtest_main.so /usr/lib/libgtest_main.so
