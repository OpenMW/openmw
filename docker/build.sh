#!/usr/bin/env bash

set -xe

# Creating build directory...
mkdir -p build
cd build

# Running CMake...
cmake ../

# Building with $NPROC CPU...
make -j $NPROC
