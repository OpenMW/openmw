#!/bin/sh -e
sudo ln -sf /usr/bin/clang-3.6 /usr/local/bin/clang
sudo ln -sf /usr/bin/clang++-3.6 /usr/local/bin/clang++

eval "${MATRIX_EVAL}"
