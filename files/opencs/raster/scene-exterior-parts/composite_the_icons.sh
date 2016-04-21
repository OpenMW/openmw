#!/bin/bash

# Run this from the "parts" directory to composite the final 32 icons
# Also creates status (masked) variants of the icons while at it

set -e

mkdir -p status
i=0

for grid in '' '*_grid*'; do
for arrows in '' '*_arrows*'; do
for cell_marker in '' '*_cell_marker*'; do
    files=$(echo $grid $arrows $cell_marker | tr ' ' '\n' | sort -n | tr '\n' ' ')
    convert *backdrop* $files *terrain* -background transparent -mosaic \
            -crop '23x48+25+0' *mask* -mosaic status/$i.png
    i=$((i+1))
done;done;done;
