#!/bin/sh -ex
sudo ln -s /usr/bin/clang-3.6 /usr/local/bin/clang
sudo ln -s /usr/bin/clang++-3.6 /usr/local/bin/clang++

# build libgtest, libgtest_main, libgmock, libgmock_main
git clone https://github.com/google/googletest.git
mkdir googletest/build
cd googletest/build
cmake ..
make -j$(nproc)
sudo make install
