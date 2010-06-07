#!/bin/bash

function run()
{
  echo
  echo "$1/tests/:"
  cd "$1/tests/"
  ./test.sh
  cd ../../
}

run tools
run input
run bsa
run nif
run nifogre
