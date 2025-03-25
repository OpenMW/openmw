#!/bin/bash -ex

mkdir example-suite
cd example-suite
git init
git remote add origin https://gitlab.com/OpenMW/example-suite.git
git fetch --depth=1 origin ${EXAMPLE_SUITE_REVISION}
git checkout FETCH_HEAD
cd ..

xvfb-run --auto-servernum --server-args='-screen 0 640x480x24x60' \
    scripts/integration_tests.py --verbose --omw build/install/bin/openmw --workdir integration_tests_output example-suite/

ls integration_tests_output/*.osg_stats.log | while read v; do
    scripts/osg_stats.py --stats '.*' --regexp_match < "${v}"
done
