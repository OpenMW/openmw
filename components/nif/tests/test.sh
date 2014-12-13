#!/bin/bash

#Script to test all nif files (both loose, and in BSA archives) in data files directory

DATAFILESDIR="$1"

find "$DATAFILESDIR" -iname *bsa > nifs.txt
find "$DATAFILESDIR" -iname *nif >> nifs.txt

sed -e 's/.*/\"&\"/' nifs.txt > quoted_nifs.txt

xargs --arg-file=quoted_nifs.txt ../../../niftest

rm nifs.txt
rm quoted_nifs.txt
