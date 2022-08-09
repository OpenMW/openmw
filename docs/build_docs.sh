#!/bin/bash

pushd $( dirname -- "$0"; )
docker run --user "$(id -u)":"$(id -g)" --volume "$PWD/..":/openmw openmw_doc \
    sphinx-build /openmw/docs/source /openmw/docs/build
popd
