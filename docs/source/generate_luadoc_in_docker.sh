#!/bin/bash -e

if ! [[ -f "/.dockerenv" ]]; then
    echo "Run this script inside Docker container" >&2
    exit -1
fi

FILES_DIR=/openmw/files
OUTPUT_DIR=/openmw/docs/source/reference/lua-scripting/generated_html

rm -rf "${OUTPUT_DIR}"
mkdir "${OUTPUT_DIR}"

cd "${FILES_DIR}/lua_api"
openmwluadocumentor -f doc -d "${OUTPUT_DIR}" openmw/*lua

cd "${FILES_DIR}/data"
openmwluadocumentor -f doc -d "${OUTPUT_DIR}" openmw_aux/*lua
openmwluadocumentor -f doc -d "${OUTPUT_DIR}" scripts/omw/ai.lua
openmwluadocumentor -f doc -d "${OUTPUT_DIR}" scripts/omw/camera/camera.lua
openmwluadocumentor -f doc -d "${OUTPUT_DIR}" scripts/omw/mwui/init.lua
openmwluadocumentor -f doc -d "${OUTPUT_DIR}" scripts/omw/settings/player.lua
