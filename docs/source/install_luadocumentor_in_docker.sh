#!/bin/bash -e

if [ ! -f /.dockerenv ] && [ ! -f /home/docs/omw_luadoc_docker ]; then
    echo 'This script installs lua-5.1, luarocks, and openmwluadocumentor to $HOME. Should be used only in docker.'
    exit 1
fi

echo "Install lua 5.1"
cd ~
curl -R -O https://gitlab.com/OpenMW/openmw-deps/-/raw/main/lua/lua-5.1.5.tar.gz
echo "2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333 lua-5.1.5.tar.gz" | sha256sum -c
tar -zxf lua-5.1.5.tar.gz
rm lua-5.1.5.tar.gz
cd lua-5.1.5/
make linux
cd ~
PATH=$PATH:~/lua-5.1.5/src

echo "Install luarocks"
luarocksV="3.9.2"
wget https://gitlab.com/OpenMW/openmw-deps/-/raw/main/lua/luarocks-$luarocksV.tar.gz
echo "bca6e4ecc02c203e070acdb5f586045d45c078896f6236eb46aa33ccd9b94edb luarocks-$luarocksV.tar.gz" | sha256sum -c
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
