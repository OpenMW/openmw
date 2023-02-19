#!/bin/bash

DOCS_SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
FILES_DIR=$DOCS_SOURCE_DIR/../../files
OUTPUT_DIR=$DOCS_SOURCE_DIR/reference/lua-scripting/generated_html
DOCUMENTOR_PATH=~/.luarocks/bin/openmwluadocumentor

if [ ! -x $DOCUMENTOR_PATH ]; then
    if [ -f /.dockerenv ] || [ -f /home/docs/omw_luadoc_docker ]; then
        ./install_luadocumentor_in_docker.sh
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

data_paths=$($DOCS_SOURCE_DIR/luadoc_data_paths.sh)

cd $FILES_DIR/lua_api
$DOCUMENTOR_PATH -f doc -d $OUTPUT_DIR openmw/*lua
cd $FILES_DIR/data
for path in $data_paths
do
  $DOCUMENTOR_PATH -f doc -d $OUTPUT_DIR $path
done
