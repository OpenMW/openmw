#!/bin/bash

function run()
{
  cd "$1/tests/"
  ./test.sh
  cd ../../
}

run stream
run vfs
run sound
run .
