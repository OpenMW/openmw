pushd .

echo "Install lua 5.1"
cd ~
curl -R -O https://www.lua.org/ftp/lua-5.1.5.tar.gz
tar -zxf lua-5.1.5.tar.gz
rm lua-5.1.5.tar.gz
cd lua-5.1.5/
make linux
cd ~
PATH=$PATH:~/lua-5.1.5/src

echo "Install luarocks"
luarocksV="3.9.2"
wget https://luarocks.org/releases/luarocks-$luarocksV.tar.gz
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
luarocks make luarocks/openmwluadocumentor-0.2.0-1.rockspec
cd ~
rm -r openmw-luadocumentor


echo "Install Teal Cyan"
git clone https://github.com/teal-language/cyan.git --depth 1
cd cyan
luarocks make cyan-dev-1.rockspec

luarocks show openmwluadocumentor
luarocks show cyan

LUAROCKS=~/luarocks/bin
export LUAROCKS
popd
pushd docs
./build_teal.sh ../build_teal
