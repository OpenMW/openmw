#!/bin/bash -e

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

if [[ -z "${DOCS_SOURCE_DIR+x}" ]]; then
    DOCS_SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
fi

FILES_DIR="${DOCS_SOURCE_DIR:?}/../../files"
OUTPUT_DIR="${DOCS_SOURCE_DIR:?}/reference/lua-scripting/generated_html"

if [[ -z "${DOCUMENTOR_PATH+x}" ]]; then
    DOCUMENTOR_PATH=~/.luarocks/bin/openmwluadocumentor
fi

if [ ! -x "${DOCUMENTOR_PATH:?}" ]; then
    # running on Windows?
    DOCUMENTOR_PATH="${APPDATA:?}/LuaRocks/bin/openmwluadocumentor.bat"
fi

rm -rf "${OUTPUT_DIR:?}"
mkdir "${OUTPUT_DIR:?}"

cd "${FILES_DIR:?}/lua_api"
"${DOCUMENTOR_PATH:?}" -f doc -d "${OUTPUT_DIR:?}" openmw/*lua

cd "${FILES_DIR:?}/data"
"${DOCUMENTOR_PATH:?}" -f doc -d "${OUTPUT_DIR:?}" openmw_aux/*lua
"${DOCUMENTOR_PATH:?}" -f doc -d "${OUTPUT_DIR:?}" scripts/omw/ai.lua
"${DOCUMENTOR_PATH:?}" -f doc -d "${OUTPUT_DIR:?}" scripts/omw/camera/camera.lua
"${DOCUMENTOR_PATH:?}" -f doc -d "${OUTPUT_DIR:?}" scripts/omw/mwui/init.lua
"${DOCUMENTOR_PATH:?}" -f doc -d "${OUTPUT_DIR:?}" scripts/omw/settings/player.lua
