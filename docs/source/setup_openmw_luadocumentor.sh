#!/bin/bash -e

echo "Install lua 5.1"
cd ~
curl -R -O https://www.lua.org/ftp/lua-5.1.5.tar.gz
tar -zxf lua-5.1.5.tar.gz
cd lua-5.1.5/
make -j $(nproc) linux
PATH="${PATH:?}:~/lua-5.1.5/src"

echo "Install luarocks"
cd ~
wget https://luarocks.org/releases/luarocks-2.4.2.tar.gz
tar zxpf luarocks-2.4.2.tar.gz
cd luarocks-2.4.2/
./configure --with-lua-bin="${HOME:?}/lua-5.1.5/src" --with-lua-include="${HOME:?}/lua-5.1.5/src" --prefix="${HOME:?}/luarocks"
make -j $(nproc) build
make -j $(nproc) install
PATH="${PATH:?}:~/luarocks/bin"

echo "Install openmwluadocumentor"
cd ~
git clone --depth 1 https://gitlab.com/ptmikheev/openmw-luadocumentor.git
cd openmw-luadocumentor/luarocks
luarocks --local pack openmwluadocumentor-0.1.1-1.rockspec
luarocks --local install openmwluadocumentor-0.1.1-1.src.rock
