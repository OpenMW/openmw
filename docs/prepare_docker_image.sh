#!/bin/bash

pushd $( dirname -- "$0"; )
docker build -t openmw_doc .
popd
