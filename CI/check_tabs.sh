#!/bin/bash

OUTPUT=$(grep -nRP '\t' --include=\*.{cpp,hpp,c,h} --exclude=ui_\* apps components)

if [[ $OUTPUT ]] ; then
    echo "Error: Tab characters found!"
    echo $OUTPUT
    exit 1
fi
