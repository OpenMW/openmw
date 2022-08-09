#!/bin/bash

DOCS_SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
FILES_DIR=$DOCS_SOURCE_DIR/../../files
OUTPUT_DIR=$DOCS_SOURCE_DIR/reference/lua-scripting/generated_html
DOCUMENTOR_PATH=~/.luarocks/bin/openmwluadocumentor

if [ ! -x $DOCUMENTOR_PATH ]; then
    if [ -f /.dockerenv ] || [ -f /home/docs/omw_luadoc_docker ]; then
        . install_luadocumentor_in_docker.sh
    else
        # running on Windows?
        DOCUMENTOR_PATH="$APPDATA/LuaRocks/bin/openmwluadocumentor.bat"
    fi
fi
if [ ! -x $DOCUMENTOR_PATH ]; then
    echo "openmwluadocumentor not found; See README.md for installation instructions."
    exit
fi

rm -f $OUTPUT_DIR/*.html

cd $FILES_DIR/lua_api
$DOCUMENTOR_PATH -f doc -d $OUTPUT_DIR openmw/*lua

cd $FILES_DIR/data
$DOCUMENTOR_PATH -f doc -d $OUTPUT_DIR openmw_aux/*lua
$DOCUMENTOR_PATH -f doc -d $OUTPUT_DIR scripts/omw/ai.lua
$DOCUMENTOR_PATH -f doc -d $OUTPUT_DIR scripts/omw/camera/camera.lua
$DOCUMENTOR_PATH -f doc -d $OUTPUT_DIR scripts/omw/mwui/init.lua
$DOCUMENTOR_PATH -f doc -d $OUTPUT_DIR scripts/omw/settings/player.lua
