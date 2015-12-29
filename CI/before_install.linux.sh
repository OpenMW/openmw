#!/bin/sh

if [ "${ANALYZE}" ]; then
  if [ $(lsb_release -sc) = "precise" ]; then
    echo "yes" | sudo apt-add-repository ppa:ubuntu-toolchain-r/test
  fi
  echo "yes" | sudo add-apt-repository "deb http://llvm.org/apt/`lsb_release -sc`/ llvm-toolchain-`lsb_release -sc`-3.6 main"
  wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key|sudo apt-key add -
fi

echo "yes" | sudo add-apt-repository "deb http://archive.ubuntu.com/ubuntu `lsb_release -sc` main universe restricted multiverse"
echo "yes" | sudo apt-add-repository ppa:openmw/openmw
echo "yes" | sudo apt-add-repository ppa:boost-latest/ppa
sudo apt-get update -qq
sudo apt-get install -qq libgtest-dev google-mock
sudo apt-get install -qq libboost-filesystem1.55-dev libboost-program-options1.55-dev libboost-system1.55-dev libboost-thread1.55-dev
sudo apt-get install -qq ffmpeg libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev
sudo apt-get install -qq libbullet-dev libopenscenegraph-dev libmygui-dev libsdl2-dev libunshield-dev libtinyxml-dev libopenal-dev libqt4-dev
sudo apt-get install -qq cmake-data #workaround for broken osgqt cmake script in ubuntu 12.04
if [ "${ANALYZE}" ]; then sudo apt-get install -qq clang-3.6; fi
sudo mkdir /usr/src/gtest/build
cd /usr/src/gtest/build
sudo cmake .. -DBUILD_SHARED_LIBS=1
sudo make -j4
sudo ln -s /usr/src/gtest/build/libgtest.so /usr/lib/libgtest.so
sudo ln -s /usr/src/gtest/build/libgtest_main.so /usr/lib/libgtest_main.so
