if [ ! -f /.dockerenv ] && [ ! -f /home/docs/omw_luadoc_docker ]; then
    echo 'This script installs lua-5.1, luarocks, and openmwluadocumentor to $HOME. Should be used only in docker.'
    exit 1
fi

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
wget https://luarocks.org/releases/luarocks-2.4.2.tar.gz
tar zxpf luarocks-2.4.2.tar.gz
rm luarocks-2.4.2.tar.gz
cd luarocks-2.4.2/
./configure --with-lua-bin=$HOME/lua-5.1.5/src --with-lua-include=$HOME/lua-5.1.5/src --prefix=$HOME/luarocks
make build
make install
cd ~
rm -r luarocks-2.4.2
PATH=$PATH:~/luarocks/bin

echo "Install openmwluadocumentor"
git clone https://gitlab.com/ptmikheev/openmw-luadocumentor.git
cd openmw-luadocumentor/luarocks
luarocks --local pack openmwluadocumentor-0.1.1-1.rockspec
luarocks --local install openmwluadocumentor-0.1.1-1.src.rock
cd ~
rm -r openmw-luadocumentor
