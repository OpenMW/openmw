#!/bin/bash

function run()
{
  echo "TESTING $1"
  cd "$1/tests/"
  ./test.sh
  cd ../../
}

run input
