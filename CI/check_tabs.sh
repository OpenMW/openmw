#!/bin/bash

OUTPUT=$(grep -nRP '\t' --include=\*.{cpp,hpp,c,h} apps components)

if [[ $OUTPUT ]] ; then
    echo "Error: Tab characters found!"
    echo $OUTPUT
    exit 1
fi
