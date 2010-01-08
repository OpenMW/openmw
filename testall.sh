#!/bin/bash

function run()
{
  cd "$1/tests/"
  ./test.sh
  cd ../../
}

run tools
run bsa
run nif
run nifogre
