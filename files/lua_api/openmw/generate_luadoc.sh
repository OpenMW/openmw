#!/bin/bash

luadocumentor -f doc -d ../../../docs/source/generated-luadoc/lua-api-reference *.lua
sed -i 's/openmw\.\(\w*\)\(\#\|\.html\)/\1\2/g' ../../../docs/source/generated-luadoc/lua-api-reference/*.html
