#!/bin/bash

# Run this from the "parts" directory to composite the final 32 icons
# Also creates status (masked) variants of the icons while at it

set -e

mkdir -p composited
mkdir -p status
i=0

for terrain in '' '*_terrain_*'; do
for fog in '' '*_fog_*'; do
for water in '' '*_water_*'; do
for pathgrid in '' '*_pathgrid*'; do
for references in '' '*_bridge*'; do
    files=$(echo $water $terrain $references $fog $pathgrid | tr ' ' '\n' | sort -n | tr '\n' ' ')
    convert *sky* $files -layers flatten composited/$i.png
    convert *sky* $files *mask* -layers flatten status/$i.png
    i=$((i+1))
done;done;done;done;done