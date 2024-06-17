#!/bin/bash -e

if [ ! -f /.dockerenv ] && [ ! -f /home/docs/omw_luadoc_docker ]; then
    echo 'This script installs lua-5.1, luarocks, and openmwluadocumentor to $HOME. Should be used only in docker.'
    exit 1
fi

echo "Install lua 5.1"
cd ~
curl -R -O https://gitlab.com/OpenMW/openmw-deps/-/raw/main/lua/lua-5.1.5.tar.gz
tar -zxf lua-5.1.5.tar.gz
rm lua-5.1.5.tar.gz
cd lua-5.1.5/
make linux
cd ~
PATH=$PATH:~/lua-5.1.5/src

echo "Install luarocks"
luarocksV="3.9.2"
wget https://gitlab.com/OpenMW/openmw-deps/-/raw/main/lua/luarocks-$luarocksV.tar.gz
tar zxpf luarocks-$luarocksV.tar.gz
rm luarocks-$luarocksV.tar.gz
cd luarocks-$luarocksV/
./configure --with-lua-bin=$HOME/lua-5.1.5/src --with-lua-include=$HOME/lua-5.1.5/src --prefix=$HOME/luarocks
make build
make install
cd ~
rm -r luarocks-$luarocksV
PATH=$PATH:~/luarocks/bin

echo "Install openmwluadocumentor"
git clone https://gitlab.com/ptmikheev/openmw-luadocumentor.git
cd openmw-luadocumentor
git checkout 78577b255d19a1f4f4f539662e00357936b73c33
luarocks make luarocks/openmwluadocumentor-0.2.0-1.rockspec
cd ~
rm -r openmw-luadocumentor
