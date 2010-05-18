#!/bin/bash

function run()
{
  echo "$1/tests/:"
  cd "$1/tests/"
  ./test.sh
  cd ../../
}

run tools
run bsa
run nif
run nifogre
