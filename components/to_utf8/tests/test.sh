#!/bin/bash

make || exit

mkdir -p output

PROGS=*_test

for a in $PROGS; do
    if [ -f "output/$a.out" ]; then
        echo "Running $a:"
        ./$a | diff output/$a.out -
    else
        echo "Creating $a.out"
        ./$a > "output/$a.out"
        git add "output/$a.out"
    fi
done
