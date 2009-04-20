#!/bin/bash

svn export ../../../monster/trunk/monster/ . --force
rm -r minibos vm/c_api.d

for a in $(find -iname \*.d); do
    cat "$a" | sed s/monster.minibos./std./g > "$a"_new
    mv "$a"_new "$a"
done

svn st

svn diff options.openmw options.d
