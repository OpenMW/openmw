#!/bin/bash

svn export https://monster-script.svn.sourceforge.net/svnroot/monster-script/trunk/monster/ . --force
rm -r minibos vm/c_api.d

for a in $(find -iname \*.d); do
    cat "$a" | sed s/monster.minibos./std./g > "$a"_new
    mv "$a"_new "$a"
done

svn st

diff options.openmw options.d || $EDITOR options.d
mv options.openmw options.openmw_last
cp options.d options.openmw

