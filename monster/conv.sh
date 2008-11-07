#!/bin/bash

for a in $(find -iname \*.d); do
    cat "$a" | sed s/monster.minibos./std./g > "$a"_new
    mv "$a"_new "$a"
done
