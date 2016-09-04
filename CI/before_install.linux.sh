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

cd ~/
git clone https://github.com/TES3MP/RakNet
cd RakNet
cmake . -DRAKNET_ENABLE_DLL=OFF -DRAKNET_ENABLE_SAMPLES=OFF -DCMAKE_BUILD_TYPE=Release
mkdir ./lib
make -j3 install
cp ./Lib/RakNetLibStatic/libRakNetLibStatic.a ./lib
cd ..

wget https://github.com/zdevito/terra/releases/download/release-2016-03-25/terra-Linux-x86_64-332a506.zip
unzip terra-Linux-x86_64-332a506.zip
