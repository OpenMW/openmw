#!/bin/bash

# How to install openmwluadocumentor:

# sudo apt install luarocks
# git clone https://gitlab.com/ptmikheev/openmw-luadocumentor.git
# cd openmw-luadocumentor/luarocks
# luarocks --local pack openmwluadocumentor-0.1.1-1.rockspec
# luarocks --local install openmwluadocumentor-0.1.1-1.src.rock

# How to install on Windows:

# install LuaRocks (heavily recommended to use the standalone package)
#   https://github.com/luarocks/luarocks/wiki/Installation-instructions-for-Windows
# git clone https://gitlab.com/ptmikheev/openmw-luadocumentor.git
# cd openmw-luadocumentor/luarocks
# open "Developer Command Prompt for VS <2017/2019>" in this directory and run:
#   luarocks --local pack openmwluadocumentor-0.1.1-1.rockspec
#   luarocks --local install openmwluadocumentor-0.1.1-1.src.rock
# open "Git Bash" in the same directory and run script:
#   ./generate_luadoc.sh

if [ -f /.dockerenv ]; then
    # We are inside readthedocs pipeline
    echo "Install lua 5.1"
    cd ~
    curl -R -O https://www.lua.org/ftp/lua-5.1.5.tar.gz
    tar -zxf lua-5.1.5.tar.gz
    cd lua-5.1.5/
    make linux
    PATH=$PATH:~/lua-5.1.5/src

    echo "Install luarocks"
    cd ~
    wget https://luarocks.org/releases/luarocks-2.4.2.tar.gz
    tar zxpf luarocks-2.4.2.tar.gz
    cd luarocks-2.4.2/
    ./configure --with-lua-bin=$HOME/lua-5.1.5/src --with-lua-include=$HOME/lua-5.1.5/src --prefix=$HOME/luarocks
    make build
    make install
    PATH=$PATH:~/luarocks/bin

    echo "Install openmwluadocumentor"
    cd ~
    git clone https://gitlab.com/ptmikheev/openmw-luadocumentor.git
    cd openmw-luadocumentor/luarocks
    luarocks --local pack openmwluadocumentor-0.1.1-1.rockspec
    luarocks --local install openmwluadocumentor-0.1.1-1.src.rock
fi

DOCS_SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
FILES_DIR=$DOCS_SOURCE_DIR/../../files
OUTPUT_DIR=$DOCS_SOURCE_DIR/reference/lua-scripting/generated_html
DOCUMENTOR_PATH=~/.luarocks/bin/openmwluadocumentor

if [ ! -x $DOCUMENTOR_PATH ]; then
  # running on Windows?
  DOCUMENTOR_PATH="$APPDATA/LuaRocks/bin/openmwluadocumentor.bat"
fi

rm -f $OUTPUT_DIR/*.html

cd $FILES_DIR/lua_api
$DOCUMENTOR_PATH -f doc -d $OUTPUT_DIR openmw/*lua

cd $FILES_DIR/data
$DOCUMENTOR_PATH -f doc -d $OUTPUT_DIR openmw_aux/*lua
$DOCUMENTOR_PATH -f doc -d $OUTPUT_DIR scripts/omw/ai.lua
$DOCUMENTOR_PATH -f doc -d $OUTPUT_DIR scripts/omw/camera.lua
$DOCUMENTOR_PATH -f doc -d $OUTPUT_DIR scripts/omw/mwui/init.lua
$DOCUMENTOR_PATH -f doc -d $OUTPUT_DIR scripts/omw/settings/player.lua
